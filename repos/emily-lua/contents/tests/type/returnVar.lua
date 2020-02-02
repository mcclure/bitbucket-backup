-- Test shadow language variables + type inference
-- Expect return 5

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.main = _fn(_type.idk, {_type.int32, _type.int8.ptr.ptr}, function()
		_var.x = 6
		_var.x = 5
		_do._return(_var.x)
	end)
end):emit()
