#!/usr/bin/python

# This is a variant of regression.py that repeats regression.py tests across revisions.
# It currently assumes that the user has checked out the mercurial version of the repo.
# Because it uses wait4, I assume it will also require you to be on not-Windows.
# Usage: ./develop/performance/perfRegression.py -a
# Tested with Python 2.6.1

import sys
import os
import subprocess
import optparse
import re
import copy
import json
import time

def printLine():
    print "-------------------------------"

def targetRelative( filename ):
    return os.path.normpath(os.path.join(targetrepo, filename))

# In testing, it appears the current process environment can be altered by subprocess.Popen.
# So make a copy before any potential modification occurs.
globalEnv = copy.deepcopy( os.environ )
globalEnv["HGPLAIN"]="1" # Protection against user hgrc changing hg behavior
def envFor(env):
    return env if env else globalEnv

def fullExec(cmd, cwd=None, env=None):
    proc = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,env=envFor(env))
    outstr, errstr = proc.communicate() # Process actually runs here
    result = proc.returncode
    return bool(result),outstr,errstr

def call(cmd, cwd=None, env=None, shell=False):
    return subprocess.call(cmd, cwd=cwd, env=envFor(env), shell=shell)

# See http://stackoverflow.com/a/28521323
def timeExec(cmd, cwd=None, env=None):
    from subprocess import Popen, STDOUT

    DEVNULL = open(os.devnull, 'wb', 0)

    proc = subprocess.Popen(cmd, stdout=DEVNULL, stderr=DEVNULL, cwd=cwd, env=envFor(env))
    _,result,resources = os.wait4(proc.pid, 0)
    return bool(result), resources.ru_utime + resources.ru_stime

targetrepo = "scratch/performance/repo"
outstats = "scratch/performance/result.json"
instats  = [outstats]
baserev = "12c627cd1ede"

srcrepo = os.path.join( os.path.join( os.path.dirname(__file__), ".."), ".." )
stddir  = "sample/test"
stdfile = "sample/test/regression.txt"

# Revisions known in past which should never be tested (tests infinite-loop, etc)
poisonnodes = ["0199223969ed6d36475530c5893a17cf19fc61d2",
               "0e26ed377eb0884abd3bf1d415b6003ffb39509e",
               "7626938bb02f0e6ac2dde64873292bb95b09df2a"]

now = time.time()

db = []

help  = "%prog -a\n"
help += "\n"
help += "Accepted arguments:\n"
help += "-a            # Test all revisions in history\n"
help += "-b [rev]      # Don't test revisions earlier than this (default "+baserev+")\n"
help += "-r [rev]      # Test listed revision[s] only\n"
help += "-f [file]     # Test listed file[s] only (default is all in regression.txt)\n"
help += "--root [path] # Set the project root\n"
help += "-i [path]     # Set in stat file  (defaults to "+instats[0]+")\n"
help += "-o [path]     # Set out stat file (defaults to "+outstats  +")\n"
help += "-t [path]     # Set tmp repo      (defaults to "+targetrepo+")\n"
help += "-v            # Print all output"

parser = optparse.OptionParser(usage=help)
for a in ["a", "v"]: # Single letter args, flags
    parser.add_option("-"+a, action="store_true")
for a in ["r", "i", "o", "t", "b", "f", "-root"]: # Long args with arguments
    parser.add_option("-"+a, action="append")

(options, cmds) = parser.parse_args()
def flag(a):
    x = getattr(options, a)
    if x:
        return x
    return []

if cmds:
    parser.error("Stray commands: %s" % cmds)

if bool( flag("a") ) == bool( flag("r") ):
    parser.error("Must specify exactly one of '-a' or '-r'")

# Extract
if flag("b"):
    baserev = flag("b")[0]
overriderev = flag("r")
overridefiles = flag("f")
if flag("root"):
    srcrepo = flag("root")[0]
if flag("i"):
    instats = flag("i")
if flag("o"):
    outstats = flag("o")[0]
if flag("t"):
    targetrepo = flag("t")[0]

# Normalize -- this should be the only stretch where starting cwd matters
srcrepo = os.path.abspath(srcrepo)
outstats = os.path.abspath(outstats)
targetrepo = os.path.abspath(targetrepo)

printLine()
for inpath in instats:
    try:
        with open(inpath) as infile:
            print "Loading stats from "+inpath
            db += json.load(infile)
    except IOError:
        print "Couldn't find/open "+inpath+", skipping load"

overriderevargs = []    # "override rev args" ... maybe i should use camelcase...
for rev in overriderev:
    overriderevargs += ["-r", rev]

# Check out repo
printLine()
print "Loading repo to "+targetrepo
if call(["mkdir", "-p", targetrepo]):
    print "Failed on mkdir-- something is very wrong"
    sys.exit(1)

if call(["hg","init"], cwd=targetrepo):
    print "Repo seems to already exist"

if call(["hg","pull"] + (overriderevargs if overriderevargs else ["-r", "tip"]) + [srcrepo], cwd=targetrepo):
    print "Could not pull requested revisions"
    sys.exit(1)

if overriderevargs:
    testrevs = overriderevargs
else:
    result,outstr,errstr = fullExec(["hg","log","-r",".","--template","{node}"], cwd=srcrepo)
    if result:
        print "Need hg id for "+srcrepo+", but couldn't get it for some reason."
        sys.exit(1)
    testrevs = ["-r", baserev+":"+outstr.strip()]

result,outstr,errstr = fullExec(["hg","log","--template",
    "{rev};{node};{date};{p1node};{p2node}\\n"] + testrevs, cwd=targetrepo)
if result:
    print "Need mercurial log for "+targetrepo+", but couldn't get it for some reason."
    sys.exit(1)
searchspace = [ line.split(";") for line in outstr.split("\n") if line ]
searchspace.sort(key=lambda x:x[0])

emilypaths = ["./package/emily", "./install/emily", "./install/bin/emily"]
sampleroot1 = "sample"
sampleroot2 = "sample/test"
regression = "regression.txt"
def targetexists(path):
    return os.path.exists( os.path.join( targetrepo, path ) )
indexcommentp = re.compile(r'#.+$', re.S) # Allow comments in .txt file
session = []

try:
    for rev,node,date,parent1,parent2 in searchspace:
        printLine()

        build = []
        def version():
            return rev+":"+node[:8]

        if node in poisonnodes:
            print "Revision "+version()+" is known bad, skipping"
            continue

        result = call(["hg","up","-r",node], cwd=targetrepo)
        if result:
            print "Could not check out revision "+version()
            continue

        # Make sure we don't confuse ourselves and run the wrong emily.
        call(["rm","-f"] + emilypaths, cwd=targetrepo)
        # Make sure we don't mess up the sanitizer. Has to use shell because wildcard.
        call("rm -f src-c/*.o", cwd=targetrepo, shell=True)
        result = call(["make","EMILY_BUILD_LOCAL=1"], cwd=targetrepo)
        if result:
            print "Could not build in revision "+version()
            continue

        # Because this script runs across all versions of the repo potentially,
        # it needs to be ready to use any path scheme the repo has ever used.
        emilyat = None
        for path in emilypaths:
            if targetexists(path):
                emilyat = path
        if not emilyat:
            print "Could not find emily executable in revision "+version()
            continue

        if   targetexists( os.path.join(sampleroot1, regression) ):
            testsat = sampleroot1
        elif targetexists( os.path.join(sampleroot2, regression) ):
            testsat = sampleroot2
        else:
            print "Could not find "+regression+" in revision "+version()

        def testpath(path):
            candidate1 = os.path.join(targetrepo, os.path.join(testsat, path))
            candidate2 = os.path.join(targetrepo, path)
            if not os.path.exists( candidate1 ) and os.path.exists(candidate2):
                return candidate2
            return candidate1

        print "Testing in "+version()
        try:
            successes = 0
            with open(testpath(regression)) as f:
                for line in f.readlines():
                    line = indexcommentp.sub("", line)
                    line = line.rstrip()
                    if line and (not overridefiles or line in overridefiles):
                        test = testpath(line)
                        # Notice no test magic-comment directives are being executed
                        result, timeSpent = timeExec([emilyat, test], cwd=targetrepo)
                        if result:
                            if flag("v"):
                                print "Skipping, failed: %s" % (line)
                        else:
                            if flag("v"):
                                print "For %s: time %s" % (line, timeSpent)
                            build.append( { 'name':line, 'time':timeSpent } )
            print "Results from %s scripts" % len(build)
            if build:
                session.append( {'rev':rev,'node':node,'date':date,'parent1':parent1,'parent2':parent2,'results':build} )
        except IOError as e:
            print "In revision "+version()+" failed to read tests:"
            print e
            continue
        except OSError as e:
            print "In revision "+version()+" failed to run emily:"
            print e
            continue
except KeyboardInterrupt:
    print "\nCtrl-C, bailing out early."

printLine()
print "Done, tested %s revisions successfully" % (len(session))

if session:
    db.append( {'run':now, 'tested':session} )

if db:
    with open(outstats, 'w') as outfile:
        print "Saving stats to "+outstats
        json.dump(db, outfile)
else:
    print "Test database is empty, not saving"
