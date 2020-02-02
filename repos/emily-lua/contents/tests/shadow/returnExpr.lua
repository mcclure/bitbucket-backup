-- Test shadow language arithmetic expressions
-- Expect return 14

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function()
		_var.x = 5
		_var.y = _var.x + 4
		_do._return(_var.x + _var.y)
	end)
end):emit()
