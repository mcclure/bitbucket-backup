-- Test type inference on arithmetic expressions (output of +)
-- Build expect failure

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function()
		_var.y = "y is a string"
		_var.x = 5
		_var.y = _var.x + 4 -- y cannot be typed both int and string
		_do._return(_var.x)
	end)
end):emit()
