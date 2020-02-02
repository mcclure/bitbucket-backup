# Types

profile experimental

from project.util import *
from project.core import *

export Type = inherit Object
	toString = "[MYSTERY TYPE]"

# TODO: Not like this
let typeDebug = function(to, frm, why)
	print
		why + " type of ", to.loc.toString
		if (to)
			" (" + to.type.toString + ")"
		else
			""
		" to ", frm.loc.toString
		if (frm)
			" (" + frm.type.toString + ")"
		else
			""
		ln

export TypedNode = inherit Node
	method unify = function(to)
		let frm = this.resolve
		to = to.resolve

		if (frm.type && to.type)
			#typeDebug frm to "Unifying"
			if (!frm.type.compatible (to.type))
				fail
					nullJoin array
						"Type error between ", frm.type, " (", frm.loc
						") and ", to.type, " (", to.loc, ")"
		elif (frm.type)
			#typeDebug to frm "Setting"
			to.type = frm.type
		elif (to.type)
			#typeDebug frm to "Setting"
			frm.type = to.type
		else
			#typeDebug frm to "Forwarding"
			frm.type = new ReferType(to)

	method unifyArgResult = function(arg, result)
		let frm = this.resolve
		if (!frm.type)
			frm.type = new FunctionType(new Val(this.loc), new Val(this.loc))
		frm.type.arg.unify arg
		frm.type.result.unify result
		
	method resolve = do
		with (this.type) match
			ReferType to = do
				let resolved = to.resolve
				if (resolved.type)
					this.type = resolved.type
				resolved
			_ = this

	method toString = "[Node " + this.loc.toString + " type " + (if (this.type) (this.type.toString) else ("{Unresolved}")) + "]"

export ReferType = inherit Type
	field to = null

	method toString = if (this.to.type)
		this.to.type.toString
	else
		"{Unresolved type at " + this.to.loc.toString + "}"

export ResolvedType = inherit Type
	method compatible = function (type) (this.compatibleTest type || type.compatibleTest this)
	method compatibleTest = function (type) (type == this)

export UnknowableType = inherit ResolvedType
	method compatibleTest = function (type) (true) # FIXME
	toString = "{Any}"

export InvalidType = inherit ResolvedType
	method compatibleTest = function (type) (false)
	toString = "{INTERNAL ERROR}"

export UnitType = inherit ResolvedType
	toString = "{Unit}"

export BooleanType = inherit ResolvedType
	toString = "{Boolean}"

export NumberType = inherit ResolvedType
	toString = "{Float}"

export AtomType = inherit ResolvedType
	toString = "{Atom}"

export StringType = inherit ResolvedType
	toString = "{String}"

export FunctionType = inherit ResolvedType
	field arg = null   # These are both TypedNodes
	field result = null
	method toString = nullJoin array
		"{Function ", this.arg.type, " -> ", this.result.type, "}"
	method compatibleTest = function (type)
		if (is FunctionType type)
			this.arg.unify (type.arg)
			this.result.unify (type.result)
			true
		else
			false

let functionTypeArgIterator = inherit Object
	field type = null
	method more = is FunctionType (this.type)
	method next = do
		let result = this.type.arg.type
		this.type = this.type.result.type
		result

export functionTypeArgIter = function(functionType)
	new functionTypeArgIterator(functionType)

export Val = inherit TypedNode
	field type = null

export KnownTypeVal = inherit Val # Use for typed literals with no other content

let standardUnknowableVal = new KnownTypeVal(null, UnknowableType)

export functionType = foldr InvalidType function(prev, next)
	new FunctionType
		new KnownTypeVal(type=prev)
		new KnownTypeVal(type=next)

export functionTypeVal = function(a)
	new KnownTypeVal
		type = functionType a