from project.util import *
from project.compiler.util import *
from project.compiler.base import
	CtypedCompiler, Chunk, IndentChunk, upgradeTemplateVal, invokeTemplate
from project.type import
	UnitType, BooleanType, NumberType, StringType

profile experimental

export CppCompiler = inherit CtypedCompiler
	scope = do
		let dict = new ChainedDict
		dict.set chainParent (current.scope)
		upgradeTemplateVal
			dict, "println", 1
			invokeTemplate "Println"
			"template <class T> void Println(T x) { std::cout << x << std::endl; }" # FIXME: way to force include <iostream> exactly once
		upgradeTemplateVal
			dict, "%", 1
			invokeTemplate "fmod"
			null
		dict

	UnitBlock = inherit (current.UnitBlock)
		method buildMain = do
			super.buildMain

			appendArray (this.source.lines) array
				"#include <iostream>\n"
				"#include <math.h>\n"
				this.defsChunk
				"int main(int argc, char **argv)"
				"{"
				new IndentChunk
					array
						this.mainChunk
				"}"

	Function = inherit (current.Function)
		method buildEntryChunk = new Chunk
			lines = array
				"unsigned int i = 0;"
				"bool run = true;"
				"while (run) {"
				new IndentChunk
					lines = array
						"switch (i) {"
						this.caseChunk
						"}"
				"}"

	SwitchBlock = inherit (current.SwitchBlock)
		method jump = function(block) # Assume goto/branchGoto are called at most once
			this.standardFallthroughJump(block)

		method condJump = function(condVal, trueBlock, falseBlock) # Assume jump/branchJump are called at most once
			this.standardFallthroughCondJump(condVal, trueBlock, falseBlock)

	method buildVarInto = function(defsChunk, value, description)
		appendArray (defsChunk.lines) array
			"static " + this.typeToString (value.resolve.type) + " " + this.valToString(value) + ";" +
				if (description)
					" // " + description
				else
					""

	method typeToString = function(type)
		with (type) match
			BooleanType = "bool"
			NumberType = "float"
			StringType = "char *"
			UnitType = "void"
			(project.type.UnknowableType) = fail "Can't translate unknowable type" # DON'T CHECK THIS LINE IN
			_ = fail "Can't translate this type"


