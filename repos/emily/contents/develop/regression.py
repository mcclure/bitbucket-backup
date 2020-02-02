#!/usr/bin/python

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
#   # Arg: --some-argument=whatever
#       Invoke interpreter with argument
#
#   # Env: SOME_ENVIRONMENT=whatever
#       Invoke interpreter with environment variable
#
#   # Omit file
#       Invoke interpreter WITHOUT test file as argument
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
stddir  = "sample/test"
stdfile = "sample/test/regression.txt"
badfile = "sample/test/regression-known-bad.txt"

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
help += "--untested  # Check repo hygiene-- list tests in sample/test not tested"

parser = optparse.OptionParser(usage=help)
for a in ["a", "A", "v", "s", "-untested"]: # Single letter args, flags
    parser.add_option("-"+a, action="store_true")
for a in ["f", "t", "r", "i"]: # Long args with arguments
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

expectp = re.compile(r'# Expect(\s*failure)?(\:?)', re.I)
linep = re.compile(r'# ?(.+)$', re.S)
inline_expectp = re.compile(r'# Expect:\s*(.+)$', re.S|re.I)
startp = re.compile(r'^', re.MULTILINE)
argp = re.compile(r'# Arg:\s*(.+)$', re.I)
envp = re.compile(r'# Env:\s*(.+)$', re.I)
kvp = re.compile(r'(\w+)=(.+)$')
omitp = re.compile(r'# Omit\s*file', re.I)

def pretag(tag, str):
    tag = "\t%s: " % (tag)
    return startp.sub(tag, str)

failures = 0

# In testing, it appears the current process environment can be altered by subprocess.Popen.
# So make a copy before any potential modification occurs.
globalEnv = copy.deepcopy( os.environ )

for filename in files:
    expectfail = False
    scanning = False
    outlines = ''
    env = None
    args = []
    omit = False
    earlyfail = False

    # Pre-scan the file for magic comments with test instructions
    with open(filename) as f:
        for line in f.readlines():
            # First determine if this is an expect directive
            expect = expectp.match(line) # Expect:
            inline = inline_expectp.match(line)
            # If the inline pattern matches and the inline body isn't empty,
            # then we're looking at an inline expect directive
            if inline and not inline.group(1).isspace():
                outlines += inline.group(1)
            # Otherwise, if it's an expect we're beginning a multiline expect
            elif expect:
                expectfail = bool(expect.group(1))
                scanning = bool(expect.group(2))
            else:
                if scanning: # If currently inside an expect block
                    outline = linep.match(line)
                    if outline:
                        outlines += outline.group(1)
                    else:
                        scanning = False

                # Only an expect directive can end an expect block
                if not scanning: # Other directives:
                    argline = argp.match(line) # Arg:
                    if argline:
                        args += [argline.group(1)]

                    envline = envp.match(line) # Env:
                    if envline:
                        if not env:
                            env = copy.deepcopy( globalEnv )
                        kvline = kvp.match( envline.group(1) )
                        if not kvline:
                            print "\tMALFORMED TEST: \"Env:\" line not of form KEY=VALUE"
                            earlyfail = True
                            break
                        env[kvline.group(1)] = kvline.group(2)

                    if omitp.match(line):
                        omit = True

    if earlyfail:
        failures += 1
        continue

    print "Running %s..." % (filename)
    try:
        proc = subprocess.Popen(stdcall+args+([] if omit else [filename]), stdout=subprocess.PIPE, stderr=subprocess.PIPE,env=env if env else globalEnv)
    except OSError as e:
        print "\nCATASTROPHIC FAILURE: Couldn't find emily?:"
        print e
        print "Make sure you ran a plain `make` first."
        sys.exit(1)
    result = proc.wait()
    outstr, errstr = proc.communicate()

    result = bool(result)
    expectfail = bool(expectfail)
    outlines = outlines.rstrip()
    outstr = outstr.rstrip()
    errstr = errstr.rstrip()

    if result ^ expectfail:
        print "\tFAIL:   Process failure " + ("expected" if expectfail else "not expected") + " but " + ("seen" if result else "not seen")
        if errstr:
            print "\n"+pretag("STDERR",errstr)
        failures += 1
    elif outstr != outlines:
        print "\tFAIL:   Output differs"
        print "\n%s\n\n%s" % ( pretag("EXPECT", outlines), pretag("STDOUT", outstr) )
        failures += 1
    elif flag("v"):
        if outstr:
            print pretag("STDOUT", outstr)
        if outstr and errstr:
            print
        if errstr:
            print pretag("STDERR",errstr)

print "\n%d tests failed of %d" % (failures, len(files))

sys.exit(0 if failures == 0 else 1)
