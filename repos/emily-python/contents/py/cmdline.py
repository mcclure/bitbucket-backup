#!/usr/bin/env python

# Entry point / command line interface

import optparse
import sys

import core
import util
import reader
import parser
import execution

# Command line frontend -- mimic optparse

help  = "{0} [filename] [program-args]\n"
help += "\n"
help += "Accepted arguments:\n"
help += "-e [string]               # Instead of file, execute inline string\n"
help += "--ast                     # Don't execute, dump AST\n"
help += "--ast2                    # Don't execute, run macros and dump AST\n"
help += "--exported                # After executing, print exported symbols"

cmdStoreTrue = ["--ast", "--ast2", "--exported"] # Single letter args, flags
cmdAppend = ["-e"] # Long args with arguments

def cmdStrip(s):
	return s.lstrip('-')
def cmdFail(msg):
	innerHelp = help.format(sys.argv[0])
	out = "%s: Error: %s\n\nUsage:\n%s\n" % (sys.argv[0], msg, innerHelp)
	sys.stderr.write(out)
	sys.exit(2)

options, target, argv = {}, None, []

def flag(a, b=None):
    if a in options:
    	return options[a]
    return []

# Parse arguments

loopLoading = None
for i in range(1,len(sys.argv)):
	arg = sys.argv[i]

	if loopLoading:
		loopLoadkey = cmdStrip(loopLoading)
		if loopLoadkey in options:
			options[loopLoadkey].append(arg)
		else:
			options[loopLoadkey] = [arg]
		loopLoading = None
	elif arg[0] == '-':
		if arg in cmdStoreTrue:
			options[cmdStrip(arg)] = True
		elif arg in cmdAppend:
			loopLoading = arg
			continue
		else:
			cmdFail("Option not recognized: %s" % (arg))
	else:
		target = arg

	if target or 'e' in options:
		argv = [util.utf8string(x) for x in sys.argv[i+1:]]
		break

if loopLoading:
	cmdFail("No such option: %s" % (loopLoading))

# Interpret parsed arguments

if flag('e'):
	if len(flag('e')) > 2:
		cmdFail("Multiple -e arguments seen")
else:
	if not target:
		cmdFail("No file given")

# TODO: Convert -e to unicode

sys.setrecursionlimit(50000)

try:
	# String to token tree
	try:
		ast = reader.ast(
			util.utf8string(flag('e')[0]) if flag('e') 
				else util.fileChars(util.utfOpen(target))
		)
	except IOError:
		print >>sys.stderr, "Compilation failed:"
		print >>sys.stderr, "Could not open file \"" + target + "\""
		sys.exit(1)

	if flag('ast'):
		print ast
		sys.exit(0)

	# Interpreter init that is needed if we got past the reader
	if not flag('e'):
		execution.setEntryFile(target)

	# Token tree to execution tree
	ast = parser.exeFromAst(ast)

	if flag('ast2'):
		print ast
		sys.exit(0)

	# Evaluate execution tree

	scope = execution.ObjectValue(execution.defaultScope)
	scope.atoms['argv'] = execution.ArrayValue(argv)
	exported = execution.ObjectValue()

	ast.eval(scope, exported)

	if flag('exported'):
		execution.debugScopeDump(exported)

except execution.ExecutionException as e:
	output = u"Execution failed:\n\t%s\n\nAt location:" % (e)
	for frame in e.stack:
		output += u"\n\t%s at %s%sline %s char %s" % (frame.what, frame.loc.file if frame.loc.file else "", ", " if frame.loc.file else "", frame.loc.line, frame.loc.char)

	print >>sys.stderr, unicode(output).encode('utf-8')
	sys.exit(1)

except core.EmilyException as e:
	print >>sys.stderr, "Compilation failed:"
	print >>sys.stderr, e
	sys.exit(1)
