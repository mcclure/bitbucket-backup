#!/usr/bin/env python

# This implements a small clone of Make with python instead of sh syntax.
# For full usage instructions, see https://bitbucket.org/runhello/makepy

import sys
import os
import os.path
import optparse
import re
import copy
import codecs
import locale
import traceback
import subprocess

### General utility functions

def stderr(str):
    str = "%s\n" % (str) # Emulate print
    sys.stderr.write(str)

def fail(str): # Non-returning
    stderr(str)
    sys.exit(1)

try:
    from pyparsing import *
except ImportError:
    fail("Internal error: In order to run make.py, you must install Pyparsing.\nTry running:\n\tsudo easy_install pyparsing")

# This script frequently uses singleton objects as tiny inline modules.
class Util: # Singleton, access as "util". See end of __init__ for symbols.
    def __init__(s):
        def listmap(a,b): # Keep python 3 map from being overly clever
            return list(map(a,b))

        def flatten(x):
            result = []
            for v in x:
                if v is not None:
                    if type(v) == list:
                        result += v
                    else:
                        result.append(v)
            return result

        def varizelist(x):
            if len(x) > 1:
                return x
            if len(x) == 1:
                return x[0]
            return None

        def tolist(x):
            if x is None:
                return []
            if type(x) == list:
                return x
            return [x]

        def tovar(x):
            if [] == x or None == x or False == x:
                return None
            if True == x:
                return "1"
            if type(x) == list:
                return varizelist(listmap(tovar, flatten(x)))
            return str(x)

        def vartostring(x):
            if x is None:
                return ''
            elif type(x) == list:
                return " ".join(x)
            return x

        py3 = sys.version_info >= (3, 0)

        # Take a path to a UTF-8 or UTF-16 file. Return an object to be used with utflines()
        def utfopen(path):
            with open(path, 'rb') as f:
                start = f.read(2) # Check first bytes for BOM
                utf16 = start.startswith(codecs.BOM_UTF16_BE) or start.startswith(codecs.BOM_UTF16_LE)
            return codecs.open(path, 'r', 'utf-16' if utf16 else 'utf-8-sig')

        # Iterate over a utfopen() object giving strings for each line
        def utflines(f):
            for line in f.readlines():
                if not py3: # Guarantee strings, even in Python 2
                    line = codecs.encode(line, 'utf-8')
                yield line

        varre = re.compile(r'^([^=]+)=(.+)$', re.S)

        # Convert a string to a valid config-file value string
        def toconfigformat(str):
            return str.replace('\\', "\\\\").replace('\n', "\\n").replace('\r', "\\r")

        def evaluatelist(statements):
            for s in statements:
                s.evaluate()

        def expressionlistfor(makefile, line):
            ev = lambda x: x.evaluate(makefile, line)
            evlist = lambda x: util.flatten(listmap(ev, x))
            return evlist

        # Name to register python code under
        def pythonname(name, makefile, line):
            return "%s in %s at %s%s" % (name, makefile.loc.makefile(), "line " if type(line)==int else "", line)

        def parseexceptiontext(e, message=None):
            if isinstance(e, ParseFatalException):
                return e
            if message is not None:
                return message
            return "This is not an assignment, rule or anything this parser recognizes"

        # FIXME: Should this be copy.deepcopy so rules can be totally isolated?
        scopecopy = copy.copy

        # Export
        s.listmap = listmap
        s.flatten = flatten
        s.varizelist = varizelist
        s.tolist = tolist
        s.tovar = tovar
        s.vartostring = vartostring
        s.py3 = py3
        s.utfopen = utfopen
        s.utflines = utflines
        s.varre = varre
        s.toconfigformat = toconfigformat
        s.evaluatelist = evaluatelist
        s.expressionlistfor = expressionlistfor
        s.pythonname = pythonname
        s.parseexceptiontext = parseexceptiontext
        s.scopecopy = scopecopy

util = Util()

### Process invocation ###

# Object to describe the location of a makefile
# Inputs: A path (as in, input of -C), a filename (as in, input of -f), optionally a Makefile this is "relative to"
class Loc(object):
    stdfile = "Make.rules"
    originalcwd = os.getcwd()

    class LocException(Exception): # FIXME: Should I really have this?
        pass

    def __init__(s, path=None, makefilename=None, relativeobj=None):
        if relativeobj:
            oldpath = relativeobj.path
        else: # If no relative object given, act relative to pwd
            oldpath = ''
        if not path:
            path = ''
        elif not os.path.isabs(path):
            path = os.path.join(oldpath, path)
        if path and not os.path.isdir(path):
            if makefilename:
                raise Loc.LocException("Requested directory %s makefile %s, but %s is a file" % (path, makefilename, path))
            path, makefilename = os.path.split(path)
        if not makefilename:
            makefilename = Loc.stdfile
        s.path = path
        s.makefilename = makefilename

    def seenkey(s):
        return (s.path, s.makefilename)

    def file(s, name):
        return os.path.normpath( os.path.join(s.path, name) )

    def makefile(s):
        return s.file(s.makefilename)

    def rebase(s):
        if s.path:
            os.chdir(s.path)
        s.path = ''

    # Allow using Loc in the with construct
    class Locwith(object):
        def __init__(s, into):
            s.into = into
        def __enter__(s):
            s.original = os.getcwd()
            if s.into:
                os.chdir(s.into)
            return s
        def __exit__(s, *_):
            os.chdir(s.original)
    def enter(s):
        return Loc.Locwith(s.path)

class Invoke: # Singleton, access as "invoke". See end of __init__ for symbols.
    def __init__(s):
        stdconfig = "Make.config"

        # Arguments

        help  = "%prog [targets] [vars]\n"
        help += "\n"
        help += "Accepted arguments:\n"
        help += "-f [filename]             # Load file instead of %s\n"%(Loc.stdfile)
        help += "    or --file [filename]\n"
        help += "--config                  # Instead of running, save vars to %s\n"%stdconfig
        help += "--add-config              # Same, but include existing contents of %s\n"%stdconfig
        help += "--config-file [filename]  # Load/save vars from file instead of %s\n"%stdconfig
        help += "-B                        # Unconditionally make all targets.\n"
        help += "    or --always-make\n"
        help += "-C [path]                 # Switch to directory before running rules\n"
        help += "    or --directory [path] # (if given a filename, will autofill -f)\n"
        help += "-i                        # Print targets to build and quit\n"
        help += "--no-version-enforcement  # Ignore Python version mismatches"

        parser = optparse.OptionParser(usage=help)
        for a in ["-config", "-add-config", "B", "-always-make", "i", "-no-version-enforcement"]: # Single letter args, flags
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

        # Remember current directory
        cwd = os.getcwd()

        # Interpret flags.
        makedir = first(flag("C"), flag("directory"), [None])[0]
        makefile = first(flag("f"), flag("file"), [None])[0]
        alwaysmake = first(flag("B"), flag("always_make"))
        configuring = first(flag("config"), flag("add_config"))
        configfile = flag("config_file")

        rulerequest = []
        cmdvars = {}

        for cmd in cmds:
            var = util.varre.match(cmd)
            if var: # So, no compiling files with = in the name...
                name, value = var.group(1).strip(), var.group(2).strip()
                cmdvars[name] = value # Note: Allows variables Python cannot understand
            else:
                rulerequest.append(cmd)

        # Path

        loc = Loc(makedir, makefile)
        loc.rebase() # Start off by permanently moving into the -C directory
        if not configfile: # Replace .rules file extension with .config
            configfile = re.sub(r'\.rules$', '', loc.makefile()) + ".config"
        configpath = loc.file(configfile)

        # Environment

        backslashre = re.compile(r'\\(.)')
        def badconfig():
            fail('Error: Configuration file %s is of an unexpected format.\nEither remove this file, or run with --config-file=""' % (configfile))
        def configescape(match):
            ch = match.group(1)
            if ch == 'n':
                return "\n"
            if ch == 'r':
                return "\r"
            if ch == '\\':
                return '\\'
            badconfig()

        # Read config format. See also toconfigformat() above
        configs = {}
        if not configuring or flag("add_config"):
            try: # Parse configuration file
                with util.utfopen(configpath) as f:
                    for line in util.utflines(f):
                        line = backslashre.sub(configescape, line.rstrip('\r\n'))
                        if line: # Always one blank line at end of file, so allow that
                            var = util.varre.match(line)
                            if var:
                                name, value = var.group(1), var.group(2) # Notice: No whitespace stripping
                                configs[name] = value
                            else:
                                badconfig()
            except IOError:
                pass

        # Command-line overrides config file, which overrides environment.
        startingenv = copy.deepcopy( os.environ )
        for key in configs:
            startingenv[key] = configs[key]
        for key in cmdvars:
            startingenv[key] = cmdvars[key]

        # Export
        s.flag = flag
        s.parsererror = parser.error
        s.loc = loc
        s.alwaysmake = alwaysmake
        s.configuring = configuring
        s.envimport = set()
        s.configfile = configfile
        s.configpath = configpath
        s.rulerequest = rulerequest
        s.env = startingenv

invoke = Invoke()

### AST definition ###

# Entire-line statements:

# Base class for statements
class Baseline(object):
    # Subclasses must call this in init
    def subinit(s):
        s.line = None
        s.builder = False
        s.descends = False
        s.elseplug = False
        s.elsesocket = False

    # Baseline interface: Parser should call after receiving object to give it its line.
    def setcontext(s, makefile, line):
        s.makefile = makefile
        s.line = line

    # Baseline interface: Parser should call when object is created and ready to be queued
    def register(s, statements):
        statements.append(s)

    def expression(s, expr):
        return expr.evaluate(s.makefile, s.line)

    def expressionlist(s, lst):
        return util.flatten(util.listmap(s.expression, lst))

# Statement: Assign variable
class Assignline(Baseline):
    def __init__(s, to, frm, isenv, israw, op):
        s.subinit()
        s.to = to
        s.frm = frm
        s.isenv = isenv
        s.israw = israw
        s.op = op

    # Baseline interface: Post-parser should call to actually cause line to take effect
    def evaluate(s):
        if not s.to.constant():
            raise ParseMakefileException("Variable names cannot contain dollar signs", s.makefile, s.line)

        key = s.expression(s.to)
        values = s.expressionlist(s.frm)

        if s.isenv: # Env changes the command completely
            if s.op != "=":
                raise ParseMakefileException("Cannot use 'env' with an operator other than =", s.makefile, s.line)
            try:
                s.makefile.envload(key, util.varizelist(values), raw=s.israw)
            except MakeException as e:
                raise ParseMakefileException(e, s.makefile, s.line)
            return

        if s.op != "=": # All possibilities except = depend on the previous value of the var:
            newvalues = values                                # Move new stuff aside, and
            values = util.tolist(util.tovar(s.makefile.globals[key] if key in s.makefile.globals else None)) # start with that
            if s.op == "?=":
                if len(values) == 0:
                    values = newvalues # Oops, we want the new values after all!
                else:
                    return # DO NOTHING
            elif s.op == "+=":
                values += newvalues
            elif s.op == "*=":
                for v in newvalues:
                    if v not in values:
                        values.append(v)
            elif s.op == "/=": # Not so sure about this one
                for v in newvalues:
                    while v in values:
                        values.remove(v)
            else:
                fail("Internal error: Assignment parser is broken")

        s.makefile.globals[key] = util.varizelist(values)

    @classmethod
    def construct(cls, p):
        member = p.member
        op = p.op
        isenv = p.isenv
        return cls(member[0],member[1:], bool(isenv), isenv=="envraw", op if op else "=")

# Statement: Prints a statement and possibly terminates the program.
class Printline(Baseline):
    def __init__(s, kind, items):
        s.subinit()
        s.kind = kind
        s.items = items

    def evaluate(s):
        message = " ".join(map(s.expression, s.items))
        if s.kind == "print":
            print(message)
        elif s.kind == "warn":
            stderr(message)
        else:
            raise ParseMakefileException(message, s.makefile, s.line)

    @classmethod
    def construct(cls, p):
        return cls(p.kind, list(p.member))

# Statement: Guarantees a particular version of Python
class Versionline(Baseline):
    def __init__(s, base, special, to):
        s.subinit()
        s.base = base
        s.special = special
        s.to = to

    def evaluate(s):
        version = list(sys.version_info)[:3]
        correct = True
        if s.special:
            if not version >= s.base:
                correct = False
                why = "too old"
            if correct and s.special == "-": # (If it isn't -, it must be +)
                if not version <= s.to:
                    correct = False
                    why = "too new"
        else:
            for idx in range(len(s.base)):
                if s.base[idx] != version[idx]:
                    correct = False
                    why = "wrong"
                    break
        if not correct:                        # Failure
            explain = "Your version of Python is %s for this makefile" % (why)
            if invoke.flag("no_version_enforcement"): # Choose to ignore failure
                stderr("Warning: Should not be using makefile %s: %s (see line %s). Trying to run anyway..." % (s.makefile.loc.makefile(), explain, s.line))
            else:
                raise ParseMakefileException(explain, s.makefile, s.line)

    @classmethod
    def construct(cls, p):
        def lister(version): # Ugly splitting because Pyparsing combine() does both too much & not enough
            return util.listmap(int, str.split(version, "."))
        to = p.to
        return cls(lister(p.base), p.special, lister(to) if to else None)

# Base class for statements which build up a python statement
class Buildline(Baseline):
    def subinit(s):
        super(Buildline, s).subinit()
        s.builder = True
        s.buildingcode = ""
        s.codecache = None
        s.codename = None

    def code(s):
        if not s.codecache:
            s.buildingcode += "\n"
            s.codename = util.pythonname(s.name(), s.makefile, s.line)
            try:
                s.codecache = compile(s.buildingcode, s.codename, mode="exec")
            except Exception as e:
                raise EmbeddedPythonException.WithCurrentException("compiling %s" % s.codename)
            s.buildingcode = None
        return s.codecache

    # Buildline interface: This object is multi-line, parser should call this to add additional lines
    def build(s,newline):
        s.buildingcode += newline

# Statement: Execute a python statement immediately
class Pythonline(Buildline):
    def __init__(s, isglobal):
        s.subinit()
        s.isglobal = isglobal

    # See Assignline
    def evaluate(s):
        scope = s.makefile.globals
        if not s.isglobal:
            scope = util.scopecopy(scope)

        with s.makefile.loc.enter():
            exec(s.code(), scope, scope)

    def name(s):
        return "global:" if s.isglobal else "do:"

    @classmethod # Construct from pyparsing result
    def construct(cls, p):
        return cls(p.kind == "global")

class Ifline(Baseline):
    def __init__(s, keyword, condition):
        s.subinit()
        s.descends = True
        s.statements = []
        s.condition = condition if condition else None
        s.alternate = None
        if keyword == "if":
            s.elsesocket = True
        elif keyword == "elif":
            s.elsesocket = True
            s.elseplug = True
        else: # keyword == "else"
            s.elseplug = True

    # Else-type ifs can interact with the previous statement, so they have a special register()
    def register(s, statements):
        if s.elseplug: # Elses don't go into the statement list normally. Instead they register with an if:
            def badsocket():
                raise ParseMakefileException("Found an else: not after an if:", s.makefile, s.line)
            if not statements: # No previous line in this block
                badsocket()
            socket = statements[-1]
            while True:
                if not socket.elsesocket: # Tried to plug an else into an else, looks like
                    badsocket()
                if socket.alternate:      # This if already has an else-- plug into that instead
                    socket = socket.alternate
                else:                     # Here's good
                    socket.alternate = s
                    break
        else: # Just act like a normal statement
            super(Ifline, s).register(statements)

    def evaluate(s):
        if s.condition is None: # This is an else:
            do = True
        else:
            do = s.condition.evaluate(s.makefile, s.line)

        if do:
            util.evaluatelist(s.statements)
        elif s.alternate: # "Else"
            s.alternate.evaluate()

    @classmethod
    def construct(cls, p):
        return cls(p.kind, p.condition)

class Forline(Baseline):
    def __init__(s, bind, values):
        s.subinit()
        s.descends = True
        s.statements = []
        s.bind = bind
        s.values = values

    def evaluate(s):
        if not s.bind.constant():
            raise ParseMakefileException("Variable bindings in 'for' should not contain dollar signs")
        bind = s.expression(s.bind)

        for x in s.expressionlist(s.values):
            # FIXME: Only s.bind once
            # FIXME: Is there any way to put this in something like a local dict?
            s.makefile.globals[bind] = x
            util.evaluatelist(s.statements)

    @classmethod
    def construct(cls, p):
        return cls(p.bind, p.valuelist)

class Importline(Baseline):
    def __init__(s, dir, file):
        s.subinit()
        s.dir = dir
        s.file = file

    def evaluate(s):
        dirpath = s.dir.evaluate(s.makefile, s.line)
        filepath = s.file.evaluate(s.makefile, s.line) if s.file else None
        loc = Loc(dirpath, filepath, s.makefile.loc)
        try:
            m = Makefile.load(loc)
        except Makefile.AlreadyLoadedException():
            raise ParseMakefileException("Makefile %s has already been loaded" % (loc.makefile()), s.makefile, s.line)
        s.makefile.imports.append((m, s.makefile, s.line)) # Save not just new makefile but where the import happened

    @classmethod
    def construct(cls, p):
        return cls(p.dir, p.file)

class Includeline(Baseline):
    def __init__(s, file):
        s.subinit()
        s.file = file

    def evaluate(s):
        file = s.file.evaluate(s.makefile, s.line)
        try:
            Makefile(Loc('', file, s.makefile.loc), clone=s.makefile).evaluate(None, nocopy=True)
        except MakeException as e:
            raise ParseMakefileException("Error in included makefile:\n%s" % e, s.makefile, s.line)

    @classmethod
    def construct(cls, p):
        return cls(p.file)

# Will be created and registered by a RuleLine
class Rule(object):
    def __init__(s, makefile, target, dep, code):
        s.makefile = makefile
        # We usually work with full paths, but during evaluation we need Makefile-local paths.
        s.innertarget = target
        s.innerdep = dep
        s.target = makefile.loc.file( s.innertarget )
        s.dep = util.listmap( makefile.loc.file, dep )
        s.code = code

    # Ruleline interface: Post-Evaluate should call this for rules that need to run
    def execute(s):
        scope = util.scopecopy(s.makefile.globals)
        scope['target'] = s.innertarget
        scope['dep'] = s.innerdep

        with s.makefile.loc.enter():
            try:
                exec(s.code, scope, scope)
            except Exception as e: # fail instead of raise because there's no one to report to right now
                fail( EmbeddedPythonException.WithCurrentException("executing rule '%s'" % s.target) )

    # Ruleline interface: Return true/false on "should we try to run this rule if we hit it?"
    # Assumes it will be called once, at a certain point in queue processing (ie, all deps have been run first)
    def should(s):
        result = False
        if invoke.alwaysmake: # Force make # TODO: OR TARGET IS PHONY
            result = True
        else:
            for dep in s.dep: # Make because one of your dependencies already made
                if dep in Makefile.dirty:
                    result = True
        if not result:
            if not os.path.exists(s.target): # Make because your target doesn't exist
                result = True
                s.mtime = 0
            else:
                s.mtime = os.path.getmtime(s.target)
                if os.path.getmtime(s.makefile.loc.makefile()) > s.mtime: # Make because the Makefile is newer than your target
                    result = True
                else:
                    for dep in s.dep: # Make because a dependency is newer than your target
                        if Makefile.rules[dep].mtime > s.mtime: # Wait, this doesn't exist for some targets. Is this safe?
                            result = True
        if result:            # Remember results for later "already made" checks
            Makefile.dirty.add(s.target)
        return result

# Statement: Registers a rule when reached
class Ruleline(Buildline):
    def __init__(s, target, dep):
        s.subinit()
        s.target = target # Will be an Expression object
        s.evaluatedtarget = None
        s.dep = dep       # Will be an array of Expression objects

    def name(s):
        return "rule '%s'" % s.evaluatedtarget

    # See Assignline
    def evaluate(s):
        s.evaluatedtarget = s.expression(s.target) # s.code() will use this indirectly. Clumsy...?
        dep = s.expressionlist(s.dep)
        rule = Rule(s.makefile, s.evaluatedtarget, dep, s.code())
        if rule.target in Makefile.rules:
            raise ParseMakefileException("Attempted to define target %s, but this is already defined in makefile %s" % (rule.target, Makefile.rules[rule.target].makefile.loc.makefile()), s.makefile, s.line)
        Makefile.rules[rule.target] = rule

    @classmethod
    def construct(cls, p):
        if p.invalid:
            raise ParseException(None)
        return cls(p[0], p[1:])

# Expressions:

class Baseexpr(object):
    @classmethod
    def construct(cls, p):
        return cls(p[0])

class Literalexpr(Baseexpr):
    def __init__(s, contents):
        s.contents = contents

    # Baseexpr interface: Post-parser should call to get string value of line
    def evaluate(s, _, _2):
        return s.contents

    # Baseexpr interface: True if constant, false if potentially dynamic.
    def constant(s):
        return True

class Varexpr(Baseexpr):
    def __init__(s, name):
        s.name = name

    # See literalexpr
    def evaluate(s, makefile, _): # Have to run through tovar in case global: messed things up
        return util.tovar(makefile.globals[s.name]) if s.name in makefile.globals else None

    def constant(s):
        return False

class Pythonexpr(Baseexpr):
    def __init__(s, code):
        s.buildingcode = code
        s.code = None
        s.codename = None

    # See literalexpr
    def evaluate(s, makefile, line):
        if not s.codename:
            s.codename = util.pythonname("$ expression", makefile, line)

        if not s.code:
            try:
                s.code = compile(s.buildingcode.lstrip(), s.codename, mode="eval")
            except Exception as e:
                raise EmbeddedPythonException.WithCurrentException("compiling %s" % s.codename)
            s.buildingcode = None

        with makefile.loc.enter():
            try:
                value = eval(s.code, makefile.globals, {})
            except Exception as e:
                raise EmbeddedPythonException.WithCurrentException("executing %s" % s.codename)

        return util.tovar(value)

    def constant(s):
        return False

class Compoundexpr(Baseexpr):
    def __init__(s, members):
        s.members = members

    def evaluate(s, makefile, line):
        result = ""
        for member in s.members:
            e = member.evaluate(makefile,line)
            if e is not None:
                if type(e) == list:
                    raise ParseMakefileException("Tried to interpolate a list into a string", makefile, line)
                result += e
        return result

    def constant(s):
        for member in members:
            if not member.constant():
                return True
        return False

    @classmethod
    def construct(cls, p):
        if len(p) == 1:
            return p
        return cls(p)

# Exceptions:
# I break this file's usual style on camel casing because these are potentially visible to users.

class MakeException(Exception):
    def __str__(s):
        return s.args[0]

class ParseMakefileException(MakeException):
    def __init__(s, mesg, makefile, line, importpoint=None):
        s.mesg = str(mesg)
        s.makefile = makefile
        s.line = line
        s.importpoint = importpoint
    def __str__(s):
        if s.importpoint:
            where = ", while processing import from line %s" % s.line
            importpoint = s.importpoint.loc.makefile()
            if importpoint != s.makefile.loc.makefile():
                where += " of included makefile %s" % importpoint
        else:
            where = " at line %s" % s.line
        return "Error parsing makefile %s%s:\n%s" % (s.makefile.loc.makefile(), where, s.mesg)

class EmbeddedPythonException(MakeException):
    def __init__(s, when, exn):
        s.when = when
        s.exn = exn
    def __str__(s):
        return "Error in embedded Python while %s:\n\n%s" % (s.when, s.exn)

    @classmethod
    def WithCurrentException(cls, *args, **kwargs):
        kwargs['exn'] = traceback.format_exc()
        return cls(*args, **kwargs)

class VarConversionError(ValueError):
    pass

class CommandException(RuntimeError):
    pass

class CommandNotFoundException(CommandException):
    pass

class CommandFailureException(CommandException):
    pass

### Define grammar ###

class Grammar: # Singleton, access as "grammar". See end of __init__ for symbols.
    def __init__(s):
        if util.py3:
            unidecoder = codecs.getdecoder('unicode_escape')
            def unidecode(x):
                return unidecoder(x)[0]
        else:
            def unidecode(x):
                return x.decode('unicode-escape')

        def quoteescapespecial(escapep, resultf):
            p = Suppress("\\") + escapep
            p.setParseAction(resultf)
            return p
        def quoteescape(escape, result):
            return quoteescapespecial(Literal(escape), lambda _:result)
        def hexarray(a):
            return int(''.join(a), 16)
        def quotehex(escape, count):
            return Regex(r'\\%s[a-fA-F0-9]{%d}' % (escape, count)).setParseAction(lambda x: unidecode(x[0]))
        def badescape(x):
            raise ParseFatalException("Don't recognize escape sequence '\\%s'" % x)

        # Patterns: Expression types

        quoteletterp = Regex(r'[^\\]') | quoteescape('n', '\n')| quoteescape('r', '\r') | quoteescape('t', '\t') \
            | quotehex('x', 2) | quotehex('u', 4) | quotehex('U', 8) \
            | quoteescapespecial(Regex(r'\w'), lambda x: badescape(x[0])) \
            | quoteescapespecial(Regex('.'), lambda x: x[0])
        quotefreep = ZeroOrMore(quoteletterp).leaveWhitespace() # Currently unused
        quotemarkp = Suppress("\"") + ZeroOrMore(NotAny("\"") + quoteletterp).leaveWhitespace() + Suppress("\"")
        quoteparenp = Forward()
        quoteparenp << ZeroOrMore( (NotAny(Literal("(") | Literal(")")) + quoteletterp) \
            | Literal("(") + quoteparenp + Literal(")") ).leaveWhitespace()
        for p in [quotefreep, quotemarkp, quoteparenp]:
            p.setParseAction(lambda x:''.join(x))
        quotep = (quotemarkp | ( Suppress("(") + quoteparenp + Suppress(")").setParseAction(lambda x:x) ))

        spacep = Suppress(Regex(r'\s+').leaveWhitespace())

        expressionbasicp = Regex(r'[^\s\(\)\"\=\:\$\#]+')
        expressionbasicp.setParseAction(Literalexpr.construct)

        envvaluebasicp = Regex(r'[^\s\(\)\"]+')
        envvaluebasicp.setParseAction(Literalexpr.construct)

        expressionquotep = quotep.copy()
        expressionquotep.setParseAction(Literalexpr.construct)

        identifiercandidatep = Regex(r'[^\W\d]\w*', re.UNICODE) # FIXME: Is this adequate for all unicode edge cases?
        expressionvarp = Combine(Suppress(Literal("$")) + identifiercandidatep)
        expressionvarp.setParseAction(Varexpr.construct)

        expressionpythonp = Combine(Suppress(Literal("$")) + quotep)
        expressionpythonp.setParseAction(Pythonexpr.construct)

        def makeCompoundExpression(basicp):
            compoundp = OneOrMore(basicp).leaveWhitespace()
            compoundp.setParseAction(Compoundexpr.construct)
            # FIXME: The NoMatch() has no effect, but for some reason is necessary for leaveWhitespace() to not complain.
            memberp = (compoundp|NoMatch()).setResultsName("member", True)
            listp = (memberp + ZeroOrMore(spacep + memberp).leaveWhitespace())
            return memberp,listp

        expressionmemberp, expressionlistp = makeCompoundExpression(expressionbasicp | expressionquotep | expressionvarp | expressionpythonp)

        # FIXME: This includes () as a quote operator,
        _, envvaluelistp = makeCompoundExpression(envvaluebasicp | expressionquotep)
        envvaluelistp = envvaluelistp + StringEnd()

        # Patterns: fail directive
        printp = oneOf("print warn error").setResultsName("kind") + (spacep + expressionlistp)
        printp.setParseAction(Printline.construct)

        # Patterns: python directive
        versionp = delimitedList( Word(nums), delim=".", combine=True )
        expressionversionp = Suppress("python") + spacep + versionp.setResultsName("base") + \
            Optional(Literal("+").setResultsName("special") | (Literal("-").setResultsName("special") + versionp.setResultsName("to")))
        expressionversionp.setParseAction(Versionline.construct)

        # Patterns: do directive
        expressioninlinep = oneOf("do global").setResultsName("kind") + Suppress(":")
        expressioninlinep.setParseAction(Pythonline.construct)

        # Patterns if directive
        expressionifp = ((oneOf("if elif").setResultsName("kind") + spacep + expressionmemberp.setResultsName("condition")) \
            | Literal("else").setResultsName("kind")) + Suppress(":")
        expressionifp.setParseAction(Ifline.construct)

        # Patterns: for directive
        expressionforp = Suppress("for") + spacep + expressionmemberp.setResultsName("bind") + spacep + Suppress("in") + spacep + \
            expressionlistp.setResultsName("valuelist") + Suppress(":")
        expressionforp.setParseAction(Forline.construct)

        # Patterns: import/include directive
        expressionimportp = Literal("import") + spacep + expressionmemberp.setResultsName("dir") + \
                Optional(spacep + Literal("file") + spacep + expressionmemberp.setResultsName("file"))
        expressionimportp.setParseAction(Importline.construct)

        expressionincludep = Literal("include") + spacep + expressionmemberp.setResultsName("file")
        expressionincludep.setParseAction(Includeline.construct)

        # Patterns: a = b
        envtagp = oneOf("env envraw").setResultsName("isenv")
        eqp = Optional(envtagp + spacep) + expressionmemberp + oneOf("= ?= += *= /=").setResultsName("op") + Optional(expressionlistp)
        eqp.setParseAction(Assignline.construct)
        envp = envtagp + spacep + expressionmemberp
        envp.setParseAction(Assignline.construct)

        # Patterns: a: b (Notice junk at start to prevent if: alone on a line)
        targetp = ((oneOf("if elif for") + ":").setResultsName("invalid") | expressionmemberp) + Suppress(":") + Optional(expressionlistp)
        targetp.setParseAction(Ruleline.construct)

        # Grammar entry point
        # Notice "specials" must go before targetp/eqp
        linep = (printp | expressionversionp | expressioninlinep | expressionifp | expressionforp
                    | expressionimportp | expressionincludep | targetp | eqp | envp) + \
                (Suppress('#' + restOfLine) | StringEnd())

        # Export
        s.linep = linep
        s.expressionlistp = expressionlistp + StringEnd()
        s.envvaluelistp = envvaluelistp + StringEnd()

grammar = Grammar()

# Feed file into grammar line by linecount

class Makefile(object):
    # Each run of this script executes one network of Makefiles.
    # These globals track the state of the network. Keys are absolute filepaths.
    rules = {}
    dirty = set()  # Serviced rules
    loaded = set() # Loaded makefiles

    # Used internally by Makefile evaluator:
    class Frame:
        def __init__(s, indent=None, statements=None, building=None):
            if (statements is None) == (building is None):
                fail("Internal error: Got confused parsing makefile")
            s.indent = indent
            s.statements = statements
            s.building = building

    class AlreadyLoadedException(Exception):
        pass

    blankre = re.compile(r'^\s*(?:#|$)')
    startingre = re.compile('^(\s*)')
    extendre = re.compile(r'\\\s*$')

    # Call this method instead of init so it can ensure each Makefile loaded only once
    @classmethod # Note how easy it would be to switch semantics to "allow recursion, but dedup" later
    def load(cls, loc):
        key = loc.seenkey()
        if key in cls.loaded:
            raise Makefile.AlreadyLoadedException()
        makefile = cls(loc)
        cls.loaded.add(key)
        return makefile

    def __init__(s, loc, clone=None):
        s.loc = loc
        if clone: # This is an 'include' makefile, and acts like more lines on an existing makefile
            s.exportenvkeys = clone.exportenvkeys
            s.globals = clone.globals
            s.originalenv = clone.originalenv
            s.exportenvcache = clone.exportenvcache
            s.imports = clone.imports
        else:     # This is a normal makefile
            s.exportenvkeys = set() # Variables that should be passed to subprocesses.
            s.globals = {}
            s.originalenv = None
            s.exportenvcache = None
            s.imports = [] # Makefiles to import at end of evaluation

    def evaluate(s, env, nocopy=False):
        if not nocopy:
            s.originalenv = copy.deepcopy(env)
            s.exportenvcache = copy.deepcopy(env)

        linecount = 0     # Lines processed from file so far
        metaline = ""     # Current backslash-extended line
        metalinecount = 0 # How long is backslash-extended line?

        statements = []  # List of Baseline objects to execute in order

        # Parse phase: Load file one line at a time
        try:
            makefilepath = s.loc.makefile()
            makemtime = os.path.getmtime(makefilepath)
            with util.utfopen(makefilepath) as f:
                parsestack = []
                def parsepush(**kwargs):
                    parsestack.append(Makefile.Frame(**kwargs))
                parsepush(statements=statements, indent='')

                for line in util.utflines(f):
                    linecount += 1 # Where are we?

                    # Report multi-line statements as the line they started on
                    linenum = linecount - metalinecount

                    # Before any processing: Stitch together lines if they end with a backslash. No comments after backslash allowed.
                    extended = s.extendre.search(line)
                    if metalinecount:
                        line = metaline + " " + line
                    if extended:
                        line = s.extendre.sub('', line)
                        metalinecount += 1
                        metaline = line
                        continue
                    else:
                        metalinecount = 0
                        metaline = ""

                    # Don't care about whitespace-only lines
                    if s.blankre.match(line):
                        continue

                    # Whitespace for this line
                    startingwhite = s.startingre.match(line).group(1)
                    frame = parsestack[-1]

                    # Manage block structure:
                    if frame.indent is None:          # This is the first line of a new block
                        previndent = parsestack[-2].indent # Only allow indent to increase
                        if startingwhite != previndent and startingwhite.startswith(previndent):
                            frame.indent = startingwhite

                    # Is this its own statement or part of a multi-line statement?
                    if frame.building is not None and frame.indent is not None and startingwhite.startswith(frame.indent): # We have a multi-line statement going and this is part of it
                        frame.building.build( line[ len(frame.indent): ] ) # Feed string direct to statement
                        continue # We are done with this line

                    if frame.indent != startingwhite: # If after all that indents still don't match, need frame rollback:
                        while True:
                            parsestack.pop()
                            if not parsestack:
                                raise ParseMakefileException("Indent does not match any previous indent amount (did you accidentally mix spaces and tabs?)", s, linenum)
                            frame = parsestack[-1]
                            if frame.indent == startingwhite:
                                break

                    line = line.rstrip() # Final cleanup on line: Right-strip whitespace

                    # Attempt to interpret the line as a statement
                    try:
                        match = grammar.linep.parseString(line)
                    except ParseBaseException as e:
                        raise ParseMakefileException(util.parseexceptiontext(e), s, linenum)

                    match = match[0] # Unpack prepared object
                    match.setcontext(s, linenum)
                    match.register(frame.statements)
                    if match.builder:    # Multi-line statement
                        parsepush(building=match)
                    elif match.descends: # Statement with child block
                        parsepush(statements=match.statements)

                while parsestack: # File done, roll back all frames
                    parsestack.pop()

        except OSError:
            fail("Make error: Could not open %s makefile '%s'" % ("default" if makefilepath == Loc.stdfile else "", makefilepath))

        # Run makefile
        s.populate()
        util.evaluatelist(statements)

        # Did we import anything?
        for (next, makefile, line) in s.imports:
            try:
                next.evaluate(s.exportenv())
            except MakeException as e:
                raise ParseMakefileException(e, s, line, importpoint=makefile)

    ### Functions to populate scope with symbols

    def envvalue(s, name, default=None, raw=False):
        env = s.originalenv
        s.exportenvkeys.add(name) # This will be published to child processes
        if name in env:
            if invoke.configuring:
                invoke.envimport.add(name)
            value = env[name]
            if not raw:
                context = "environment variable %s" % (name)
                expressionlist = util.expressionlistfor(s, context)
                try:
                    value = grammar.envvaluelistp.parseString(value)
                except ParseBaseException as e:
                    raise MakeException("Error interpreting environment variable %s:\n%s" % (name, util.parseexceptiontext(e, "Could not parse inner quotes")))
                value = util.varizelist(expressionlist(value))
            return value
        else:
            return default

    def envload(s, name, default=None, raw=False):
        s.globals[name] = s.envvalue(name, default, raw)

    def exportenv(s):
        env = s.exportenvcache
        for key in s.exportenvkeys:
            value = s.globals[key]
            env[key] = util.vartostring(value) # Quirk: Can only write strings into this object
        return env

    # Block scope trick: This function creates the populate() function then returns it
    def makepopulate():
        def tostring(x):
            return " ".join( map(str, x) )

        def single(x):
            if x is None or type(x) == list:
                raise VarConversionError("Expected a single value, but got %s", x)
            return x

        def dir(x):
            return os.path.dirname(x)

        def touch(filenames):
            for filename in util.tolist(filenames):
                import os
                # Based on recommendation from http://stackoverflow.com/a/1158096
                with open(filename, 'a'):    # Ensure file existence
                    os.utime(filename, None) # Update timestamp

        def populate(s):
            def exportvalue(key):
                if key in s.exportenvkeys:
                    return s[key]
                else:
                    return s.originalenv[key]

            def q(line):
                try:
                    p = grammar.expressionlistp.parseString(line)
                except ParseBaseException as e:
                    raise MakeException("Error parsing string interpolation (\"q()\"):\n%s" % util.parseexceptiontext(e))
                expressionlist =  util.expressionlistfor(s, "string interpolation")
                try:
                    result = expressionlist(p)
                except MakeException as e:
                    raise EmbeddedPythonException.WithCurrentException("evaluating string interpolation (\"q()\")")
                return util.varizelist(result)

            # Can raise CommandNotFoundException or CommandFailureException on failure running,
            # or ValueError on gibberish arguments.
            def shell(cmd, args=None, cwd=None, env=None, get=None, out=None, err=None, appendout=False, appenderr=False, ignorefail=False, encoding="utf-8"):
                # Test arguments for coherence
                if cmd is None:
                    raise ValueError("Trying to run shell command, but cmd is null")
                if type(cmd) == list and type(args) == list:
                    raise ValueError("Trying to run shell command, but both cmd and args are lists")

                # Test arguments for unsupported modes
                doubleerror = "Called shell() requesting %s be both written to a file and returned. Only one is supported."
                if out and (get == "stdout" or get == "exact"):
                    raise ValueError(doubleerror % ("stdout"))
                if err and get == "exact":
                    raise ValueError(doubleerror % ("stderr"))
                if encoding == 'bytes' and get=='stdout':
                    raise ValueError("Trying to run shell command, but encoding=bytes and get=stdout are not supported together")
                if appendout and not out:
                    raise ValueError("Trying to run shell command, but appendout and out=None do not make sense together")
                if appenderr and not err:
                    raise ValueError("Trying to run shell command, but appenderr and err=None do not make sense together")

                # Massage arguments
                cmd = util.tolist(cmd) + util.tolist(args)
                if env is None:
                    env = s.exportenv()
                if encoding == 'default':
                    encoding = locale.getpreferredencoding()

                # Where does output go?
                closeout, closeerr = None, None
                if get is None: # Allow to print to stderr/out
                    stdout, stderr = None, None
                else:           # make.py should print output
                    stdout, stderr = subprocess.PIPE, subprocess.PIPE
                if out:
                    closeout = open(out, "a" if appendout else "w")
                    stdout = closeout
                if err:
                    closeerr = open(err, "a" if appenderr else "w")
                    stderr = closeerr

                # Area where closeout/closeerr are potentially used
                try:
                    # Run command
                    try:
                        proc = subprocess.Popen(cmd, stdout=stdout, stderr=stderr, cwd=cwd, env=env)
                    except OSError as e:
                        raise CommandNotFoundException("Command `%s` does not exist or is not an excecutable" % (cmd[0]))
                    outstr, errstr = proc.communicate()
                    resultcode = proc.wait()

                    # Potential early bailout for process failure
                    if (get is None or get=="stdout") and resultcode and not ignorefail:
                        raise CommandFailureException("Command `%s` failed with exit code %d" % (cmd[0], resultcode))

                    # Process results: errstr/outstr not needed
                    if get is None:
                        return
                    if get=="success":
                        return not resultcode

                    # Process results: errstr/outstr needed
                    # The output of subprocess is reinterpreted as UTF-8. This is because unicode in a Make.rules in Python 2 gets
                    # converted into UTF-8 bytes, so this increases the chance of similar behavior when running in Python 2 vs 3.
                    suppressdecode = encoding == "bytes" or (not util.py3 and encoding == "utf-8")
                    if outstr and not suppressdecode:
                        outstr = codecs.decode(outstr, encoding)
                        if not util.py3:
                            outstr = codecs.encode(outstr, "utf-8")
                    if errstr and not suppressdecode:
                        errstr = codecs.decode(errstr, encoding)
                        if not util.py3:
                            outstr = codecs.encode(outstr, "utf-8")

                    if get=="exact":
                        return (resultcode, outstr, errstr)
                    if get =="stdout":
                        result = outstr.split("\n") # FIXME: Does this break on Windows?
                        while result and not result[-1]:
                            result.pop()
                        return util.tovar(result)

                    raise ValueError("Called shell() requesting '%s' which is not a recognized value" % (get))
                finally:
                    if closeout:
                        closeout.close()
                    if closeerr:
                        closeerr.close()

            def shellwith(get):
                def shellforward(*args, **kwargs):
                    kwargs['get'] = get
                    return shell(*args, **kwargs)
                return shellforward

            s.globals['tolist'] = util.tolist
            s.globals['tovar']  = util.tovar
            s.globals['tostring'] = tostring
            s.globals['single'] = single
            s.globals['touch'] = touch
            s.globals['envvalue'] = s.envvalue
            s.globals['exportvalue'] = exportvalue
            s.globals['q'] = q
            s.globals['shell'] = shell
            s.globals['shellget'] = shellwith("stdout")
            s.globals['shellcheck'] = shellwith("success")
            s.globals['shellexact'] = shellwith("exact")
            s.globals['CommandException'] = CommandException
            s.globals['CommandNotFoundException'] = CommandNotFoundException
            s.globals['CommandFailureException'] = CommandFailureException
        return populate
    populate = makepopulate()

### Evaluate starting Makefile

try:
    Makefile.load(invoke.loc).evaluate(invoke.env)
except MakeException as e:
    fail( e )

### --config mode ###

if invoke.configuring: # After parsing Makefiles, we store environment variables and quit
    if invoke.rulerequest:
        invoke.parsererror("You requested these targets:\n%s\nBut you can't build a target and use --config at once."
            % (",".join(invoke.rulerequest)))

    try:
        with codecs.open(configpath, 'w', 'utf-8') as f:
            for key in sorted(list(invoke.envimport)):
                f.write("%s=%s\n" % (key, util.toconfigformat(originalenv[key])))
    except IOError:
        fail("Error: Could not write configure file %s" % (configfile))

    sys.exit(0) # Don't act on targets

### Act on targets ###

if not invoke.rulerequest:
    if "all" not in Makefile.rules:
        fail("This Makefile does not have a default target; you must specify one")
    invoke.rulerequest = ["all"]
for rule in invoke.rulerequest:
    if rule not in Makefile.rules:
        fail("Error: Don't know how to make the target '%s'" % (rule))
already = set() # Processed targets
queue = [invoke.rulerequest]  # A stack of "targets to process next" queues
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
    if next not in Makefile.rules:
        errtext = "Error: Don't know how to make the target '%s'" % (next)
        while processing:
            errtext += "\n\t...needed to build '%s'" % (processing.pop())
        fail(errtext + "\nHalting.")
    nextrule = Makefile.rules[next] # Rule for target
    for dep in nextrule.dep:     # Check dependencies of this target
        if dep not in already:
            blockers.append(dep)
        if dep in processing: # Note GNU make would have just printed a warning here
            fail("Error: Rule '%s' has a circular dependency on '%s', halting" % (nextrule.target, dep)) # TODO: Raise exception?
    if blockers: # Push blockers onto queue stack
        processing.append(next)
        queue.append(blockers)
    else: # We have actually found a rule to execute
        should = nextrule.should()
        if invoke.flag("i"):   # We're in print mode
            print ( "%s%s" % (next, " (will make)" if should else "") )
        else:
            if should:
                nextrule.execute()
        already.add(next)
        frame.pop() # Done with this target
