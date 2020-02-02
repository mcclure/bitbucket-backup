#!/usr/bin/python

# Graphs output of perfRegression.py
# Usage: ./develop/performance/graph.py -a
# Tested with Python 2.7.9

import sys
import json
import optparse

inpath  = "scratch/performance/result.json"
outpath = "scratch/performance/graph.png"

help  = "%prog -a\n"
help += "\n"
help += "Accepted arguments:\n"
help += "-a            # Test all revisions in history\n"
help += "-r [rev]      # Test listed revision[s] only\n"
help += "-f [files]    # Test listed file[s] only\n"
help += "-i [path]     # Set in stat file (defaults to "+inpath+")\n"
help += "-o [path]     # Set out image (defaults to "+inpath+")\n"
help += "-v            # No graph, just print and quit"

parser = optparse.OptionParser(usage=help)
for a in ["a", "v"]: # Single letter args, flags
    parser.add_option("-"+a, action="store_true")
for a in ["r", "f", "i", "o"]: # Long args with arguments
    parser.add_option("-"+a, action="append")

(options, cmds) = parser.parse_args()
def flag(a):
    x = getattr(options, a)
    if x:
        return x
    return []

if cmds:
    parser.error("Stray commands: %s" % cmds)

if not (flag("a") or flag("r") or flag("f")):
    parser.error("Must specify -a, -r or -f")

# Extract
overriderev = flag("r")
overridefiles = flag("f")
if flag("i"):
    inpath = flag("i")[0]
if flag("o"):
    outpath = flag("o")[0]

with open(inpath) as infile:
    db = json.load(infile)

def matchrev(rev, node):
    if not overriderev or node in overriderev:
        return True
    for overridenode in overriderev:
        if node.startswith(overridenode):
            return True
    return False

def matchfile(path):
    return not overridefiles or file in overridefiles

extract = {}
filenames = {}

for run in db:
    for build in run['tested']:
        testRev  = build['rev']
        testNode = build['node']
        if not matchrev(testRev,testNode):
            continue

        if testRev in extract:
            revStruct = extract[testRev]
        else:
            revStruct  = { 'date':build['date'], 'data':{} }
            needSave = True
        data = revStruct['data']

        for test in build['results']:
            name = test['name']
            if not matchfile(name):
                continue
            filenames[name] = 1
            testtime = test['time']
            if name in data:
                data[name].append(testtime)
            else:
                data[name] = [testtime]

        if needSave and revStruct:
            extract[testRev] = revStruct

if not extract:
    print "No data found for this revision"
    sys.exit(1)

# Clean up
for rev in extract:
    revdata = extract[rev]['data']
    for name in revdata:
        sum = 0
        count = 0
        for x in revdata[name]:
            sum += x
            count += 1
        revdata[name] = sum / float(count)
filenames = sorted(filenames)
revnames = sorted(extract, key=int)

if flag("v"):
    for filename in filenames:
        print filename+":"
        for rev in revnames:
            revdata = extract[rev]['data']
            if filename in revdata:
                print "\t%s" % (revdata[filename])
        print ""

    print "Covered revisions:"
    print ", ".join(revnames)

    sys.exit(0)

from pylab import plot, xlabel, ylabel, title, grid, savefig, show

for filename in filenames:
    x = []
    y = []
    for rev in revnames:
        revdata = extract[rev]['data']
        if filename in revdata:
            x.append(rev)
            y.append(revdata[filename])
    plot(x,y)

xlabel('revision')
ylabel('time')
title('Automated test runtime')
grid(True)
savefig(outpath)
show()
