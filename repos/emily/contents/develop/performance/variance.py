#!/usr/bin/python

# Loads a file and tests run-to-run consistency for a single revision
# Usage: ./develop/performance/variance.py -r 08f5b0c8369
# Tested with Python 2.6.1

import sys
import json
import optparse

inpath = "scratch/performance/result.json"

help  = "%prog -r [rev]\n"
help += "\n"
help += "Accepted arguments:\n"
help += "-r [rev]      # Test listed revision[s] only\n"
help += "-i [path]     # Set in stat file (defaults to "+inpath+")"

parser = optparse.OptionParser(usage=help)
for a in []: # Single letter args, flags
    parser.add_option("-"+a, action="store_true")
for a in ["r", "i"]: # Long args with arguments
    parser.add_option("-"+a, action="append")

(options, cmds) = parser.parse_args()
def flag(a):
    x = getattr(options, a)
    if x:
        return x
    return []

if cmds:
    parser.error("Stray commands: %s" % cmds)

if not (flag("r")):
    parser.error("Must specify -r")

# Extract
target = flag("r")[0]
if flag("i"):
    inpath = flag("i")[0]

with open(inpath) as infile:
    db = json.load(infile)

files = {}

for run in db:
    for build in run['tested']:
        testRev  = build['rev']
        testNode = build['node']
        if target == testRev or testNode.startswith(target):
            for test in build['results']:
                name = test['name']
                if name not in files:
                    files[name] = []
                files[name].append( test['time'] )

if not files:
    print "No data found for this revision"
    sys.exit(1)

metmax = None

for name in sorted(files):
    min = None
    max = None
    for test in files[name]:
        if min is None or test < min:
            min = test
        if max is None or test > max:
            max = test
    variance = max-min
    if metmax is None or variance > metmax:
        metmax = variance
    print "%s:\t%s" % (name, variance)

print "\nGreatest variance:\t%s"%(metmax)
