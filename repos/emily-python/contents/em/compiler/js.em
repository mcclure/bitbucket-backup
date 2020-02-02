from project.util import *
from project.compiler.util import *
from project.compiler.base import
	ClikeCompiler, Chunk, IndentChunk, upgradeTemplateVal, invokeTemplate

profile experimental

export JsCompiler = inherit ClikeCompiler
	scope = do
		let dict = new ChainedDict
		dict.set chainParent (current.scope)
		upgradeTemplateVal
			dict, "println", 1
			invokeTemplate "console.log"
			null
		dict

	UnitBlock = inherit (current.UnitBlock)
		method buildMain = do
			this.defsChunk = new Chunk
			this.mainChunk = new Chunk

			appendArray (this.source.lines) array # Javascript: It Apparently Has No Syntax
				this.defsChunk
				this.mainChunk

	Function = inherit (current.Function)
		method buildEntryChunk = new Chunk
			lines = array
				"let i = 0;"
				"let run = true;"
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
			"let" + " " + this.valToString(value) + ";" +
				if (description)
					" // " + description
				else
					""


