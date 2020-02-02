-- Test type inference on arithmetic expressions (input to +)
-- Build expect failure

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function()
		_var.x = "oops"
		_var.y = "oops2"
		_do._return(_var.x + _var.y) -- Can't add strings
	end)
end):emit()
