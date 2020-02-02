# Self-hosting interpreter for e2

profile experimental

from project.util import *
from project import (reader, parser, execution)
from project.compiler import
	cs.CsCompiler, cpp.CppCompiler, js.JsCompiler, debug.TypePrinter

let cmdAst = false
let cmdAst2 = false
let cmdExported = false
let cmdExecute = null
let cmdTarget = null
let cmdValid = false
let cmdOutput = null
let cmdDriver = "interpreter"

let scriptArgv = array()

# --- Parse arguments ---

do
	let i = argv.iter
	let nextArg = function(expect)
		if (!i.more)
			stderr.println
				"Missing argument for -" + expect
			exit 2
		i.next

	while (i.more)
		let arg = i.next

		with arg match
			"--ast" =
				cmdAst = true
			"--ast2" =
				cmdAst2 = true
			"--exported" =
				cmdExported = true
			"-e" =
				cmdExecute = nextArg "e"
			"-d" = # TODO: Or "driver"
				cmdDriver = nextArg "d"
			"-o" =
				cmdOutput = nextArg "o"
			_ = do
				if ("-" == arg 0)
					stderr.print "Unrecognized argument" arg ln
					exit 2
				cmdTarget = arg

		if (cmdExecute || cmdTarget)
			while (i.more)
				scriptArgv.append(i.next)
			cmdValid = true

if (!cmdValid)
	stderr.println "Must supply either file name or -e"
	exit 2

# --- Run ---

let codeIter = do
	if cmdTarget
		file.in cmdTarget
	else
		cmdExecute.iter

let ast = reader.makeAst codeIter null

if cmdTarget
	execution.setEntryFile cmdTarget
	codeIter.close

if cmdAst
	println (ast.toString)
else
	let exe = parser.exeFromAst ast

	if cmdAst2
		println (exe.toString)
	else
		let exePrint = function(s)
			if (cmdOutput == null || cmdOutput == "-")
				println s
			else
				let x = file.out cmdOutput
				x.write s
				x.close

		with cmdDriver match
			"interpreter" = do
				if (cmdOutput)
					stderr.println "-o not understood when running interpreter"
					exit 2

				from execution import (ObjectValue, StringValue, ArrayValue, defaultScope)
				let scope = new ObjectValue(defaultScope)

				let scriptArgvValue = array()
				let i = scriptArgv.iter
				while (i.more)
					scriptArgvValue.append (new StringValue(i.next))
				scope.atoms.set "argv" (new ArrayValue(scriptArgvValue))
				
				let exportScope = new ObjectValue

				exe.evalSequence(scope, exportScope)

				if cmdExported
					execution.debugScopeDump(exportScope)

			"cs" = do
				let x = CsCompiler.build exe

				exePrint x

			"cpp" = do
				let x = CppCompiler.build exe

				exePrint x

			"js" = do
				let x = JsCompiler.build exe

				exePrint x

			"type" = do
				let x = TypePrinter.build exe

				exePrint x
