#!/usr/bin/python

# This implements a small clone of Make with python instead of sh syntax.

# FEATURE PLANS:
# * Multiple targets
# * "phony" equivalents: track-separately, dont-trust-freshness, default
# * Parenthesis, inlining, lists

import sys
import os
import os.path
import optparse
import re
import copy

stdfile   = "Make.rules"
stdconfig = "Make.config"

# Based on advice in http://stackoverflow.com/questions/2544972/how-can-get-python-isidentifer-functionality-in-python-2-6
identifierp = None
def isidentifier(name):
    global identifierp
    if (sys.version_info > (3, 0)):
        return str.isidentifier(name)
    else:
        if not identifierp:
            import tokenize
            identifierp = re.compile('^%s$' % tokenize.Name)
        return identifierp.match(name)
def validname(name):
    from keyword import iskeyword
    return isidentifier(name) and not iskeyword(name)

### Parse args ###

help  = "%prog [targets] [vars]\n"
help += "\n"
help += "Accepted arguments:\n"
help += "-f [filename]             # Load file instead of %s\n"%(stdfile)
help += "    or --file [filename]\n"
help += "--config                  # Instead of running, save vars to %s\n"%stdconfig
help += "--config-file [filename]  # Load/save vars from file instead of %s\n"%stdconfig
help += "-B                        # Unconditionally make all targets.\n"
help += "    or --always-make\n"
help += "-C [path]                 # Switch to directory before running rules\n"
help += "    or --directory [path]\n"
help += "-i                        # Print targets to build and quit"

parser = optparse.OptionParser(usage=help)
for a in ["-config", "B", "-always-make", "i"]: # Single letter args, flags
    parser.add_option("-"+a, action="store_true")
for a in ["f", "-file", "-config-file", "C", "-directory"]: # Long args with arguments
    parser.add_option("-"+a, action="append")

(options, cmds) = parser.parse_args()
def flag(a, b=None):
    x = getattr(options, a)
    if x:
        return x
    return []
def first(*l):
    for i in l:
        if i:
            return i
    return None

# chdir before *anything*
runindir = first(flag("C"), flag("directory"))
if runindir is not None:
    os.chdir(runindir[0])

# Interpret flags. # FIXME: What happens if you run multiple times?
makefile = first(flag("f"), flag("file"), [stdfile])[0]
configfile = first(flag("config_file"), [stdconfig])[0]
alwaysmake = first(flag("B"), flag("always_make"))
makemtime = os.path.getmtime(makefile)

rules = []
cmdvars = {}

varp = re.compile(r'^([^=]+)=(.+)$', re.S)

for cmd in cmds:
    var = varp.match(cmd)
    if var: # So, no compiling files with = in the name...
        name, value = var.group(1), var.group(2)
        if not isidentifier(name):
            parser.error("Tried to assign to variable which is not legal in Python: %s" % name)
        cmdvars[name] = value

if flag("config"):
    parser.error("I don't have that feature yet")

### Parse file ###

import pyparsing

# Simple patterns. TODO: Replace with pyparsing
blankp = re.compile(r'^\s*(?:#|$)', re.S)
startingp = re.compile('^(\s*)')
eqp = re.compile(r'^([^=\s]+)\s*=\s*(.+)$', re.S)
targetp = re.compile(r'^([^:\s]+)\s*:\s*(\S.*)?$', re.S)
whitesplitp = re.compile('\s+')

statements = []  # List of Baseline objects to execute in order
linecount = 0    # Lines processed from file so far
rules = {}       # Map of target names to Ruleline objects
dirty = set()    # Rules that have been found to need making
phony = ["all", "clean"] # Rules that should not have corresponding sentinels

makeglobals = {"phony":phony} # Global scope dictionary while executing rules

# Base class for lines to execute
class Baseline(object):
    def subinit(s, line):
        s.line = line

    # Baseline interface: Parser should call when object is created and ready to be queued
    def register(s):
        statements.append(s)

# Assign variable
class Assignline(Baseline):
    def __init__(s, line, to, frm):
        s.subinit(line)
        s.to = to
        s.frm = frm

    # Baseline interface: Post-parser should call to actually cause line to take effect
    def evaluate(s, scope):
        scope[s.to] = s.frm

# Register a rule
class Ruleline(Baseline):
    def __init__(s, line, target, dep):
        s.subinit(line)
        s.target = target
        s.dep = whitesplitp.split(dep) if dep else []
        s.buildingcode = ""
        s.code = None

    # See Baseline
    def register(s):
        s.buildingcode += "\n"
        s.code = compile(s.buildingcode, "%s (rule %s)" % (makefile, s.target), mode="exec")
        s.buildingcode = None
        super(Ruleline, s).register()

    # See Assignline
    def evaluate(s, scope):
        rules[s.target] = s

    # Ruleline interface: This object is multi-line, parser should call this to add additional lines
    def build(s,newline):
        s.buildingcode += newline

    # Ruleline interface: Post-Evaluate should call this for rules that need to run
    def execute(s):
        exec(s.code, makeglobals, {})

    # Ruleline interface: Return true/false on "should we try to run this rule if we hit it?"
    # Assumes it will be called once, at a certain point in queue processing (ie, all deps have been run first)
    def should(s):
        result = False
        if alwaysmake or s.target in phony: # Force make
            result = True
        else:
            for dep in s.dep: # Make because one of your dependencies already made
                if dep in dirty:
                    result = True
        if not result:
            if not os.path.exists(s.target): # Make because your target doesn't exist
                result = True
                s.mtime = 0
            else:
                s.mtime = os.path.getmtime(s.target)
                if makemtime > s.mtime: # Make because the Makefile is newer than your target
                    result = True
                else:
                    for dep in s.dep: # Make because a dependency is newer than your target
                        if rules[dep].mtime > s.mtime: # Wait, this doesn't exist for some targets. Is this safe?
                            result = True
        if result:            # Remember results for later "already made" checks
            dirty.add(s.target)
        return result

class ParseMakefileException(Exception):
    pass

class ProcessRuleException(Exception):
    pass

# Parse phase: Load file one line at a time
with open(makefile) as f:
    building = None # Current multi-line Baseline being constructed (if any)
    indent = None   # Current expected indent (if any)
    for line in f.readlines():
        linecount += 1 # Where are we?

        # Don't care about whitespace-only lines
        if blankp.match(line):
            continue

        # Whitespace for this line
        startingwhite = startingp.match(line).group(1)

        # Is this its own statement or part of a multi-line statement?
        if building is not None: # We have a multi-line statement going
            if startingwhite:    # We are part of a multi-line statement
                if indent is None:             # We are the first indented line of the statement
                    indent = startingwhite
                elif indent != startingwhite:  # We are indented, but incorrectly
                    raise ParseMakefileException("Bad indent at line d" % (linecount))
                # Add this line to the statement
                building.build( line[ len(startingwhite): ] )
                continue # We are done with this line
            else: # We are not part of the statement, which means the statement is over
                building.register() # ...and can be closed
                building = None
                indent = None
        elif startingwhite: # Can't indent outside a multiline statement
            raise ParseMakefileException("Indent where none expected at line %d" % (linecount))
        line = line.rstrip() # Final cleanup on line: Right-strip whitespace

        # Attempt to interpret the line as a statement
        match = eqp.match(line)
        if match: # Assign statement
            Assignline(linecount, match.group(1), match.group(2)).register()
        else:
            match = targetp.match(line)
            if match: # Rule statement
                building = Ruleline(linecount, match.group(1), match.group(2))
            else: # Syntax error
                raise ParseMakefileException("This is not an assignment, rule or comment (line %d)" % (linecount))
    if building: # If file ends inside a rule (likely)
        building.register()

### "Execute" file

preface = """
global tolist
def tolist(x):
    if x is None:
        return []
    elif type(x) is list:
        return x
    else:
        return [x]

global tostring
def tostring(x):
    return " ".join( map(str, x) )

global touch
def touch(filenames):
    for filename in tolist(filenames):
        import os
        # Based on recommendation from http://stackoverflow.com/a/1158096
        with open(filename, 'a'):    # Ensure file existence
            os.utime(filename, None) # Update timestamp
"""

# Run preamble
exec( compile(preface, "make.py builtin code", mode="exec"), makeglobals, {} )

# Run makefile lines
for s in statements:
    s.evaluate(makeglobals)

### Act on targets ###

if not cmds:
    if "all" not in rules:
        parser.error("This Makefile does not have a default target; you must specify one")
    cmds = ["all"]
for cmd in cmds:
    if cmd not in rules:
        parser.error("Don't know how to make the target '%s'" % (cmd))
already = set() # Processed targets
queue = [cmds]  # A stack of "targets to process next" queues
processing = [] # Targets that gave rise to current top of queue

while queue:
    frame = queue[-1] # Top of queue stack
    if not frame: # Done with this frame
        if processing:
            processing.pop()
        queue.pop()
        continue
    next = frame[-1]    # Pick one target, try to operate on it
    if next in already: # Skip this target
        frame.pop()
        continue
    blockers = []          # Rules found that we must process before this one
    nextrule = rules[next] # Rule for target
    for dep in nextrule.dep:     # Check dependencies of this target
        if dep not in already:
            blockers.append(dep)
        if dep in processing: # Note GNU make would have just printed a warning here
            raise ProcessRuleException("Rule '%s' has a circular dependency, halting" % (dep))
    if blockers: # Push blockers onto queue stack
        processing.append(next)
        queue.append(blockers)
    else: # We have actually found a rule to execute
        should = nextrule.should()
        if flag("i"):   # We're in print mode
            print "%s%s" % (next, " (will make)" if should else "")
        else:
            if should:
                nextrule.execute()
        already.add(next)
        frame.pop() # Done with this target
