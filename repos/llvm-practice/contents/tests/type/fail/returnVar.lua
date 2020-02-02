-- Test shadow language variables + type inference
-- Build expect failure

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function()
		_var.x = "oops"
		_do._return(_var.x) -- x is string but return value is int
	end)
end):emit()
