#!/usr/bin/python

# This is a simple test harness script. It targets source files, extracts expected
# results from codes in the comments, and verifies the target runs as expected.
# This is adapted from a similar script in the Emily interpreter repo, but is more
# complex since it runs in two steps (test files produce an exe which is then run).
#
# Recognized comment codes:
#
#   # Testcase
#       Start a new testcase within the file (only needed for multiple tests one file)
#
#   # Testcase testname
#       Start a new testcase with a known name
#
#   # File main2
#       Executable to run (defaults to main)
#
#   # Expect failure
#       Executable should fail
#
#   # Expect return 13
#       Executable should fail with code 13
#
#   # Expect:
#   # SOMETHING
#   # SOMETHING
#       Expect "SOMETHING\nSOMETHING" as program output. (Notes: First space of
#       line always consumed; trailing whitespace of output always disregarded;
#       a non-comment line is assumed to end the Expect block; put this last.)
#
#   # Arg: --some-argument=whatever
#       Invoke interpreter with argument. Repeat for multiple arguments.
#
#   # Env: SOME_ENVIRONMENT=whatever
#       Invoke interpreter with environment variable. Repeat for multiple vars
#
#   # Build file SOMEFILE.lua
#       For .em files, specify build script
#
#   # Build expect failure
#   # Build expect return 13
#       Build step should fail / with code
#
#   # Build arg: --some-argument=whatever
#   # Build env: SOME_ENVIRONMENT=whatever
#       Invoke interpreter with argument / env
#

# Usage: ./develop/regression.py -a
# Tested with Python 2.6.1

import sys
import os
import subprocess
import optparse
import re
import copy

def projectRelative( filename ):
    return os.path.normpath(os.path.join(prjroot, filename))

prjroot = os.path.join( os.path.dirname(__file__), ".." )
stddir  = "tests"
stdfile = "tests/regression.txt"
badfile = "tests/regression-known-bad.txt"

help  = "%prog -a\n"
help += "\n"
help += "Accepted arguments:\n"
help += "-f [filename.em]  # Check single file\n"
help += "-t [filename.txt] # Check all paths listed in file\n"
help += "-r [path]         # Set the project root\n"
help += "-a          # Check all paths listed in standard " + stdfile + "\n"
help += "-A          # Also check paths listed in " + badfile + "\n"
help += "-v          # Print all output\n"
help += "-i [path]   # Use custom emily interpreter\n"
help += "-s          # Use system emily interpreter\n"
help += "--luajit [path] # Use custom Luajit path"
help += "--cc [path] # Use custom C compiler"
help += "--untested  # Check repo hygiene-- list tests in sample/test not tested"

parser = optparse.OptionParser(usage=help)
for a in ["a", "A", "v", "s", "-untested"]: # Single letter args, flags
    parser.add_option("-"+a, action="store_true")
for a in ["f", "t", "r", "i", "-luajit", "-cc"]: # Long args with arguments
    parser.add_option("-"+a, action="append")

(options, cmds) = parser.parse_args()
def flag(a):
    x = getattr(options, a)
    if x:
        return x
    return []

if cmds:
    parser.error("Stray commands: %s" % cmds)

indices = []
files = []

if flag("r"):
    prjroot = flag("r")[0]

if flag("a") or flag("A"):
    indices += [projectRelative(stdfile)]

if flag("A"):
    indices += [projectRelative(badfile)]

luajit = flag("luajit") if flag("luajit") else "luajit"
cc = flag("cc") if flag("cc") else "cc" # FIXME: Should I just use $(CC)?

indices += flag("t")

indexcommentp = re.compile(r'#.+$', re.S) # Allow comments in .txt file
for filename in indices:
    dirname = os.path.dirname(filename)
    with open(filename) as f:
        for line in f.readlines():
            line = indexcommentp.sub("", line)
            line = line.rstrip()
            if line:
                files += [projectRelative(os.path.join(dirname, line))]

files += flag("f")

if not files:
    parser.error("No files specified")

if flag("untested"):
    def checkTested(path): # Checks a directory tree rooted here
        if os.path.isdir(path): # If directory
            for filename in os.listdir(path): # Recurse
                filepath = os.path.join(path, filename)
                checkTested(filepath)
        elif not (path.endswith(".txt") or path in files):
            print path # If a normal file, just check it
    checkTested(projectRelative(stddir)) # Check stddir tree
    sys.exit(0)

stdcall = [projectRelative("install/bin/emily")]
if flag("i") and flag("s"):
    parser.error("Can't specify both -i and -s")
if flag("i"):
    stdcall = flag("i")
if flag("s"):
    stdcall = ["emily"]

startp = re.compile(r'^', re.MULTILINE)
garbagep = re.compile(r'[\s\\\"\$\`]', re.MULTILINE)

# There are two types of test file in this repo:
# - Lua files: Execute this file, link the output, then execute that
# - Emily files: Execute the compiler script on this file, link the output, execute that
# The two TestParser subclasses mediate the comment operators + the slightly different invocations for these two cases.
# Note the Emily case is slightly overkill at this moment since there is no single standard Emily compiler.
class TestParser:
    def __init__(s):
        comment = s.comment() + ' ' # Prepend to all patterns
        build = r'(Build)?\s*'
        s.testcasep      = re.compile(comment +         r'Testcase\s*(.*)$', re.I)
        s.filep          = re.compile(comment + build + r'File\s*(.+)$', re.I)
        s.expectp        = re.compile(comment + build + r'Expect(\s*failure|\s*return\s*\d+|\:)\s*$', re.I)
        s.linep          = re.compile(comment +         r'?(.+)$', re.S)       # Commented line
        s.inline_expectp = re.compile(comment + build + r'Expect:\s*(\S.*)$', re.S|re.I)
        s.argp           = re.compile(comment + build + r'Arg:\s*(.+)$', re.I)
        s.envp           = re.compile(comment + build + r'Env:\s*(.+)$', re.I)
        s.kvp            = re.compile(                  r'(\w+)=(.+)$')        # Key/value

    # Returns 2D array of tests -> (name, steps -> (stepname, invokeline, env, result [False | int | rstripped string]))
    # or None if "could not read". 
    def parseFile(s, filename):
        # Scan the file for magic comments with test instructions
        stepdefaults = {'expect':None, 'outlines':"", 'file':None, 'env':{}, 'args':[]}
        # Build a 'state' structure. It contains two batches of variables, entry 0 is build step, 1 is run step
        sharedstate = None # Common to all testcases in file
        teststate = [copy.deepcopy(stepdefaults), copy.deepcopy(stepdefaults)] # Current testcase
        testcases = [[None, teststate]] # List of testcase names + testcases
        scanning = None
        def stateFor(re):
            if re.group(1):
                return teststate[1]
            return teststate[0]

        with open(filename) as f:
            for line in f.readlines():
                if scanning: # First check if currently inside an expect block
                    match = s.linep.match(line)
                    if match:
                        scanning['outlines'] += match.group(1)
                    else:
                        scanning = None
                    continue

                match = s.testcasep.match(line) # New testcase
                if match:
                    testname = match.group(1)
                    if not sharedstate:
                        sharedstate = copy.deepcopy(teststate)
                        testcases[0][0] = testname
                    else:
                        teststate = copy.deepcopy(sharedstate)
                        testcases.append([testname, teststate])
                    continue

                match = s.filep.match(line) # Filename to execute
                if match:
                    stateFor(match)['file'] = match.group(2)
                    continue

                match = s.inline_expectp.match(line) # Inline expect
                if match:
                    state = stateFor(match)
                    state['expect'] = True
                    state['outlines'] += match.group(2)
                    continue

                match = s.expectp.match(line) # Expect failure / Expect 23 / Expect:
                if match:
                    state = stateFor(match)
                    target = match.group(2).lstrip()
                    if target == ':':
                        scanning = state
                    elif target.lower() == 'failure':
                        state['expect'] = False
                    else:
                        state['expect'] = int(target.lstrip('return').lstrip())
                    continue
                
                match = s.filep.match(line)
                if match:
                    stateFor(match)['file'] = match.group(2).rstrip()
                    continue

                match = s.argp.match(line) # Arg:
                if match:
                    stateFor(match)['args'] += [match.group(2).rstrip()]
                    continue

                match = s.envp.match(line) # Env:
                if match:
                    state = stateFor(match)
                    if not s['env']:
                        state['env'] = copy.deepcopy( globalEnv )
                    kvline = s.kvp.match( match.group(2).rstrip() )
                    if not kvline:
                        print "\tMALFORMED TEST: \"Env:\" line not of form KEY=VALUE"
                        return None
                    state['env'][kvline.group(1)] = kvline.group(2)
                    continue

        result = []
        for testcase in testcases:
            try:
                result.append( ( testcase[0], s.formatStep(filename, testcase[1]) ) )
            except TestParser.TestParserSubclassException:
                return nil
        return result

    class TestParserSubclassException(Exception):
        pass

class LinkingParser(TestParser):
    def formatStep(s, filename, spec):
        def result(step):
            return step['outlines'] if step['expect'] is None else step['expect']
        exename = spec[0]['file'] if spec[0]['file'] else "main"
        return [("build", s.executable(filename, spec) + spec[1]['args'], spec[1]['env'], result(spec[1])),
                ("link", [cc, exename+'.o', '-o', exename], None, True),
                ("run", ['./'+exename] + spec[0]['args'], spec[0]['env'], result(spec[0]))]

class EmilyParser(LinkingParser):
    def comment(s):
        return "#"
    def executable(s, filename, spec):
        return [luajit, spec[1]['file'], filename]
emilyParser = EmilyParser()

class LuaParser(LinkingParser):
    def comment(s):
        return "--"
    def executable(s, filename, spec):
        if spec[1]['file']:
            print "\tMALFORMED TEST: 'Build file' doesn't mean anything on a Lua test"
            raise TestParser.TestParserSubclassException()
        return [luajit, filename]
luaParser = LuaParser()

def parseFile(filename):
    if filename.endswith(".lua"):
        parser = luaParser
    else:
        parser = emilyParser
    return parser.parseFile(filename)

def pretag(tag, str):
    tag = "\t%s: " % (tag)
    return startp.sub(tag, str)

def printable(ary):
    result = []
    for item in ary:
        if garbagep.match(item):
            result.append('"' +
                item.replace('\\', '\\\\').replace('"', '\\"').replace('$', '\\$').replace('`', '\\`') +
                '"')
        else:
            result.append(item)
    return " ".join(result)

failures = 0
attempts = 0

# In testing, it appears the current process environment can be altered by subprocess.Popen.
# So make a copy before any potential modification occurs.
globalEnv = copy.deepcopy( os.environ )

for filename in files:
    filetests = parseFile(filename)

    if not filetests:
        failures += 1
        continue
    
    testsrun = 0

    for testspec in filetests:
        testname, test = testspec
        stepsrun = 0
        testsrun += 1
        attempts += 1
        print "Running %s%s..." % (filename, 
            ((", testcase \"%s\""%testname) if testname else ", testcase %d" % testsrun) if len(filetests) > 1 else "")

        for stepspec in test:
            stepsrun += 1
            stepname, invokeline, env, expect = stepspec

            def steptag():
                if len(test) <= 0:
                    return ""
                if stepname:
                    return ", %s step" % stepname
                return ", step %d" % stepsrun

            try:
                if flag("v"):
                    print printable(invokeline)
                proc = subprocess.Popen(invokeline, stdout=subprocess.PIPE, stderr=subprocess.PIPE,env=env if env else globalEnv)
            except OSError as e:
                print "\nCATASTROPHIC FAILURE%s: Couldn't find executable?:" % steptag()
                print e
                sys.exit(1)
            outstr, errstr = proc.communicate()
            result = proc.wait()

            expectfail = expect is False
            outstr = outstr.rstrip()
            errstr = errstr.rstrip()

            wantoutstr = flag("v")
            wanterrstr = flag("v")
            failed = False

            if type(expect) == int:
                if result != expect:
                    print "\tFAIL%s: Process exit value %d expected, but got %d" % (steptag(), expect, result)
                    wanterrstr = True
                    failed = True
            elif bool(result) ^ expectfail:
                print "\tFAIL%s: Process failure %s but %s" % (steptag(), "expected" if expectfail else "not expected", "seen" if result else "not seen")
                wanterrstr = True
                failed = True
            elif type(expect) == str and outstr != expect.rstrip():
                print "\tFAIL%s: Output differs" % steptag()
                print "\n%s\n\n%s" % ( pretag("EXPECT", expect), pretag("STDOUT", outstr) )
                wantoutstr = False
                failures += 1

            wantoutstr = wantoutstr and outstr
            wanterrstr = wanterrstr and errstr

            if wantoutstr:
                print pretag("STDOUT", outstr)
            if wantoutstr and wanterrstr:
                print
            if wanterrstr:
                print pretag("STDERR",errstr)

            if failed:
                failures += 1
                break

            # As a special case, execution does not proceed after known failures.
            # Notice specific error codes do not get this same treatment. Maybe they should?
            if expectfail:
                break

print "\n%d tests failed of %d" % (failures, attempts)

sys.exit(0 if failures == 0 else 1)
