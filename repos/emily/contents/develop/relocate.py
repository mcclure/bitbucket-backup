#!/usr/bin/python

# This script moves one or more files while preserving keeping mercurial and
# regression.txt up to date.
# Requires python 2.7.

import sys
import subprocess
import optparse
import re
import os.path

help  = "%prog oldname newname\n"
help  = "%prog -f movelist\n"
help += "\n"
help += "Accepted arguments:\n"
help += "-f [movelist.txt] # Given a list of paths separated by spaces, move all\n"
help += "                  # (assumes one pair per line, items may be quoted)\n"
help += "-g                # Use git not mercurial\n" # Not tested.
help += "-v                # Print back names while running\n"
help += "-r [rename.txt]   # Replace paths in this file"
help += "                  # (Will always attempt this in ./regression.txt,"
help += "                  # and ./regression-known-bad.txt)"

parser = optparse.OptionParser(usage=help)
for a in ["g", "v"]: # Single letter args, flags
    parser.add_option("-"+a, action="store_true")
for a in ["f", "r"]: # Long args with arguments
    parser.add_option("-"+a, action="append")

(options, cmds) = parser.parse_args()
def flag(a):
    x = getattr(options, a)
    if x:
        return x
    return []

questionp = re.compile(r'^\s*\?', re.S)
pairp = re.compile( r'^(\"[^\"]+\"|[^\s\"]+)\s+(\"[^\"]+\"|[^\s\"]+)$' )
quotep = re.compile(r'^\"(.+)\"$')

def fileIsVersioned(p):
    if flag("g"):
        status = subprocess.check_output(["git", "status", "--porcelain", p])
    else:
        status = subprocess.check_output(["hg", "status", p])
    return not questionp.match(status)

def ensurePathExists(p):
    subprocess.check_output(["mkdir", "-p", os.path.dirname(p)])

def normalMove(a, b):
    ensurePathExists(b)
    subprocess.check_output(["mv", a, b])

def versionedMove(a, b):
    if flag("g"):
        ensurePathExists(b)
        subprocess.check_output(["git", "mv", a, b])
    else:
        subprocess.check_output(["hg", "mv", a, b])

def move(a,b):
    if fileIsVersioned(a):
        versionedMove(a,b)
    else:
        normalMove(a,b)

def unquote(s):
    return quotep.sub(r'\g<1>', s)

if len(cmds)%2:
    parser.error("Odd number of arguments, refusing to run. Did you forget -f?")
    sys.exit(1)

pairs = []
for i in range(0,len(cmds),2):
    pairs += [ (cmds[i], cmds[i+1]) ]

for filename in flag("f"):
    with open(filename) as f:
        for line in f.readlines():
            result = pairp.match(line)
            if result:
                pairs += [ (unquote(result.group(1)),unquote(result.group(2))) ]
            else:
                print "FAIL: Couldn't parse line: " + line

reassign = {}

if not pairs:
    parser.error("No files given, refusing to run.")
    sys.exit(1)

for pair in pairs:
    a,b = pair
    action = "\"" + a + "\" to \"" + b + "\""
    try:
        if flag("v"):
            print "Moving " + action
        move(a,b)
        reassign[a] = b
    except subprocess.CalledProcessError:
        print "FAIL: Couldn't move " + action

for replacePath in (["regression.txt", "regression-known-bad.txt"] + flag("r")):
    try:
        newContents = ""
        anyMatches = False
        with open(replacePath) as f:
            for line in f.readlines():
                path = line.strip()
                if path in reassign:
                    newContents += ( reassign[path] + "\n" )
                    anyMatches = True
                else:
                    newContents += line
        if anyMatches:
            with open(replacePath, "w") as f:
                f.write(newContents)
    except IOError:
        print "FAIL: Couldn't edit file: " + replacePath