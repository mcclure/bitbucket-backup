-- Test variables of unknown type
-- Build expect failure

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function()
		_var.x = _var.y -- What are these?
		_do._return(0)
	end)
end):emit()
