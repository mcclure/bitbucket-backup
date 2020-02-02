-- Test type inference on arithmetic expressions (input of + with return)
-- Build expect failure

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.main = _fn(_type.int8.ptr, {_type.int32, _type.int8.ptr.ptr}, function()
		_var.x = 5
		_var.y = _var.x + 4
		_do._return(_var.x + _var.y) -- x is int but return value is string
	end)
end):emit()
