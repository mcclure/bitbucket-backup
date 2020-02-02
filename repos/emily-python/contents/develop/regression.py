#!/usr/bin/env python

# This is a simple test harness script. It targets Emily files, extracts expected
# results from codes in the comments, and verifies the script runs as expected.
#
# Recognized comment codes:
#
#   # Expect failure
#       Emily interpreter should fail
#
#   # Expect:
#   # SOMETHING
#   # SOMETHING
#       Expect "SOMETHING\nSOMETHING" as program output. (Notes: First space of
#       line always consumed; trailing whitespace of output always disregarded;
#       a non-comment line is assumed to end the Expect block; put this last.)
#
#   # Expect file: path/to/file.txt
#       Expect the contents of file.txt as program output. (Notes: Filename
#       is whitespace stripped; trailing whitespace in file disregarded.)
#
#   # Arg: --some-argument=whatever
#       Invoke interpreter with argument
#
#   # AppArg: --some-argument=whatever
#       Invoke application with argument
#
#   # Env: SOME_ENVIRONMENT=whatever
#       Invoke interpreter with environment variable
#
#   # Omit file
#       Invoke interpreter WITHOUT test file as argument
#
#   # Tags: SOMETHING SOMETHING
#       Special tags (driver-specific, may impact whether runs)

# Usage: ./develop/regression.py -a
# Tested with Python 2.6.1

import sys
import os
import subprocess
import optparse
import re
import copy
import codecs

def projectRelative( filename ):
    return os.path.normpath(os.path.join(prjroot, filename))

prjroot = os.path.join( os.path.dirname(__file__), ".." )
stddir  = "test"
stdfile = "test/regression.txt"
drivername = "default"

help  = "%prog -a\n"
help += "\n"
help += "Accepted arguments:\n"
help += "-f [filename.em]  # Check single file\n"
help += "-t [filename.txt] # Check all paths listed in file\n"
help += "--root [path]     # Set the project root\n"
help += "-d          # Driver (default, meta, cs, cpp, js, all)\n"
help += "-a          # Check all paths listed in standard " + stdfile + "\n"
help += "-A          # Run tests even if they are known bad\n"
help += "-v          # Print all output\n"
help += "-V          # Print all invocations and output\n"
help += "-i [path]   # Use custom emily script (when running Emily)\n"
help += "-p [path]   # Use custom Python (when running Python)\n"
help += "--p3        # Use python3\n"
help += "--md        # \"Meta depth\" (nest how many interpreters?)\n"
help += "--untested  # Check repo hygiene-- list tests in sample/test not tested"

parser = optparse.OptionParser(usage=help)
for a in ["a", "A", "v", "V", "-p3", "-untested"]: # Single letter args, flags
    parser.add_option("-"+a, action="store_true")
for a in ["f", "t", "d", "i", "p", "-root", "-md"]: # Long args with arguments
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

verbose = False
verboseinvoke = False
verboseskip = False
testall = False

if flag("root"):
    prjroot = flag("root")[0]

if flag("a") or flag("A"): # FIXME
    if flag("f"): # This is needed so "testall" makes sense. Would it make more sense to have an -F?
        parser.error("Don't understand simultaneous -a and -f")
    indices += [projectRelative(stdfile)]

if flag("A") or flag("f"):
    testall = True

if flag("d"):
    drivername = flag("d")[0]

if flag("v"):
    verbose = True

if flag("V"):
    verbose = True
    verboseinvoke = True

indices += flag("t")

# Take a path to a UTF-8 or UTF-16 file. Return an object to be used with utflines()
def utfOpen(path):
    with open(path, 'rb') as f:
        start = f.read(2) # Check first bytes for BOM
        utf16 = start.startswith(codecs.BOM_UTF16_BE) or start.startswith(codecs.BOM_UTF16_LE)
    return codecs.open(path, 'r', 'utf-16' if utf16 else 'utf-8-sig')
indexcommentp = re.compile(r'#.+$', re.S) # Allow comments in .txt file

for filename in indices:
    dirname = os.path.dirname(filename)
    with utfOpen(filename) as f:
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

stdpython = "python"
stdscript = "emily.py"
stdmeta = "./em/emily.em"

if flag("p") and flag("p3"):
    parser.error("Can't specify both -p and --p3")
if flag("i"):
    stdscript = flag("i")
if flag("p"):
    stdpython = flag("p")
elif flag("p3"):
    stdpython = "python3"

stdmetalist = [stdmeta]
if flag("md"):
    stdmetalist *= int(flag("md")[0])

stdcall = [stdpython, stdscript]
stdmeta = stdcall + stdmetalist

expectp = re.compile(r'# Expect(\s*failure)?(\:?)', re.I)
linep = re.compile(r'# ?(.+)$', re.S)
inline_expectp = re.compile(r'# Expect(\s*file)?:\s*(.+)$', re.S|re.I)
startp = re.compile(r'^', re.MULTILINE)
argp = re.compile(r'# (App)?Arg:\s*(.+)$', re.I)
envp = re.compile(r'# Env:\s*(.+)$', re.I)
kvp = re.compile(r'(\w+)=(.+)$')
omitp = re.compile(r'# Omit\s*file', re.I)
tagsp = re.compile(r'# Tags:\s*(.+)$', re.I)

def pretag(tag, str):
    tag = u"\t%s: " % (tag)
    return startp.sub(tag, str)

def printstd(outstr, errstr):
    if outstr:
        print pretag(u"STDOUT", outstr)
    if outstr and errstr:
        print
    if errstr:
        print pretag(u"STDERR",errstr)

failures = 0

# In testing, it appears the current process environment can be altered by subprocess.Popen.
# So make a copy before any potential modification occurs.
globalEnv = copy.deepcopy( os.environ )

# This is not the best way to use OO; only a bit of state persists in the object,
# and the object "resets" itself each time you run a file.
class BaseRunner(object):
    def __init__(s):
        s.failures = 0
        s.trials = 0

    def phases(s):
        return 1

    def should(s):
        for x in s.tags:
            for y in s.name() + ["all"]:
                if x == "skip-" + y or x == "broken-" + y:
                    return False
        return True

    def phasename(s, phase):
        return "Running"

    def phaseenv(s, phase):
        return s.env

    def normalargs(s):
        return s.args+([] if s.omit else [s.filename])

    def phaseinvoke(s, phase): # Subclasses must reimplement
        raise RuntimeError()

    def phaserun(s, phase):
        env = s.phaseenv(phase)
        invoke = s.phaseinvoke(phase)

        print "%s %s..." % (s.phasename(phase), s.filename)
        try:
            if verboseinvoke:
                print "\t$ " + " ".join(invoke)
            proc = subprocess.Popen(invoke, stdout=subprocess.PIPE, stderr=subprocess.PIPE,env=env if env else globalEnv)
        except OSError as e:
            print "\nCATASTROPHIC FAILURE: Couldn't find emily?:"
            print e
            print "Make sure you ran a plain `make` first."
            sys.exit(1)
        result = proc.wait()
        outstr, errstr = proc.communicate()

        result = bool(result)
        outstr = codecs.decode( outstr.rstrip(), 'utf-8' )
        errstr = codecs.decode( errstr.rstrip(), 'utf-8' )

        lastphase = phase == s.phases() - 1

        if (result ^ s.expectfail) if lastphase else (result and not s.expectfail):
            print "\tFAIL:   Process failure " + ("expected" if s.expectfail else "not expected") + " but " + ("seen" if result else "not seen")
            printstd(outstr, errstr)
            return False
        elif lastphase and outstr != s.outlines:
            print "\tFAIL:   Output differs"
            print u"\n%s\n\n%s" % ( pretag(u"EXPECT", s.outlines), pretag(u"STDOUT", outstr) )
            return False
        elif verbose:
            printstd(outstr, errstr)

        return True

    def run(s, filename):
        # Reset object state
        s.filename = filename
        s.expectfail = False
        s.env = None
        s.args = []
        s.appargs = []
        s.omit = False
        s.outlines = ''
        s.tags = []

        scanning = False
        earlyfail = False
        
        # Pre-scan the file for magic comments with test instructions
        with utfOpen(filename) as f:
            for line in f.readlines():
                # First determine if this is an expect directive
                expect = expectp.match(line) # Expect:
                inline = inline_expectp.match(line)
                # If the inline pattern matches and the inline body isn't empty,
                # then we're looking at an inline expect directive
                if inline and not inline.group(2).isspace():
                    if inline.group(1) and not inline.group(1).isspace(): # "Expect file"
                        s.outlines += open(inline.group(2).strip()).read().rstrip()
                    else:
                        s.outlines += inline.group(2)
                # Otherwise, if it's an expect we're beginning a multiline expect
                elif expect:
                    s.expectfail = bool(expect.group(1))
                    scanning = bool(expect.group(2))
                else:
                    if scanning: # If currently inside an expect block
                        outline = linep.match(line)
                        if outline:
                            s.outlines += outline.group(1)
                        else:
                            scanning = False

                    # Only an expect directive can end an expect block
                    if not scanning: # Other directives:
                        argline = argp.match(line) # Arg:
                        if argline:
                            arggroup = [argline.group(2)]
                            if argline.group(1):
                                s.appargs += arggroup
                            else:
                                s.args += arggroup

                        envline = envp.match(line) # Env:
                        if envline:
                            if not s.env:
                                s.env = copy.deepcopy( globalEnv )
                            kvline = kvp.match( envline.group(1) )
                            if not kvline:
                                print "\tMALFORMED TEST: \"Env:\" line not of form KEY=VALUE"
                                earlyfail = True
                                break
                            env[kvline.group(1)] = kvline.group(2)

                        elif omitp.match(line):
                            s.omit = True

                        else:
                            tagline = tagsp.match(line) # Tags:
                            if tagline:
                                s.tags = tagline.group(1).split()

        if earlyfail:
            s.trials += 1
            s.failures += 1
            return False

        if testall or s.should():
            s.trials += 1
            s.outlines = s.outlines.rstrip()
            s.expectfail = bool(s.expectfail)

            for phase in range(s.phases()):
                if not s.phaserun(phase):
                    s.failures += 1
                    break
        else:
            if verboseskip:
                print("Skipping %s..." % (s.filename))

        return True

    def runall(s):
        for filename in files:
            s.run(filename)

        print "\n%d tests failed of %d" % (s.failures, s.trials)

class NormalRunner(BaseRunner):
    def name(s):
        return ["default"]

    def phaseinvoke(s, phase):
        return stdcall + s.normalargs() + s.appargs

class MetaRunner(BaseRunner):
    def name(s):
        return ["meta"]

    def phaseinvoke(s, phase):
        return stdmeta + s.normalargs() + s.appargs

class IncompleteRunner(BaseRunner): # Compiler does not work well right now, so it uses whitelist
    def should(s):
        if not super(IncompleteRunner, s).should():
            return False
        for x in s.tags:
            for y in s.name():
                if x == y:
                    return True
        return False

class ClikeRunner(IncompleteRunner):
    def phases(s):
        return 4

    def phasename(s, phase):
        return ["Preparing", "Transpiling", "Compiling", "Running"][phase]

class CsRunner(ClikeRunner):
    def name(s):
        return ["compiler", "cs"]

    def phaseinvoke(s, phase): # TODO: mkdtemp for Windows support
        if phase == 0:
            return ["rm", "-f", "/tmp/test.cs", "/tmp/test.exe"]
        elif phase == 1:
            return stdmeta + ["-d", "cs", "-o", "/tmp/test.cs"] + s.normalargs()
        elif phase == 2:
            return ["csc", "/nologo", "-out:/tmp/test.exe", "/tmp/test.cs"]
        elif phase == 3:
            return ["mono", "/tmp/test.exe"] + s.appargs

class CppRunner(ClikeRunner):
    def name(s):
        return ["compiler", "cpp"]

    def phaseinvoke(s, phase):
        if phase == 0:
            return ["rm", "-f", "/tmp/test.cpp", "/tmp/test"]
        elif phase == 1:
            return stdmeta + ["-d", "cpp", "-o", "/tmp/test.cpp"] + s.normalargs()
        elif phase == 2:
            return ["c++", "-o", "/tmp/test", "/tmp/test.cpp"]
        elif phase == 3:
            return ["/tmp/test"] + s.appargs

class JsRunner(IncompleteRunner):
    def name(s):
        return ["compiler", "js"]

    def phases(s):
        return 3

    def phasename(s, phase):
        return ["Preparing", "Transpiling", "Running"][phase]

    def phaseinvoke(s, phase):
        if phase == 0:
            return ["rm", "-f", "/tmp/test.js"]
        elif phase == 1:
            return stdmeta + ["-d", "js", "-o", "/tmp/test.js"] + s.normalargs()
        elif phase == 2:
            return ["node", "/tmp/test.js"] + s.appargs

drivers = {
    "default" : NormalRunner(),
    "meta" : MetaRunner(),
    "cs" : CsRunner(),
    "cpp" : CppRunner(),
    "js" : JsRunner(),
}

failures = 0
trials = 0

if drivername == "all":
    for drivername in drivers:
        driver = drivers[drivername]
        print("Running: " + drivername)
        
        driver.runall()
        failures += driver.failures
        trials += driver.trials

    print "\nTotal: %d tests failed of %d" % (failures, trials)
else:
    if drivername in drivers:
        driver = drivers[drivername]
        driver.runall()
        failures = driver.failures
    else:
        parser.error("Unrecognized driver name " + drivername)

sys.exit(0 if failures == 0 else 1)
