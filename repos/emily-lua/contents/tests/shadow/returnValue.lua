-- Minimal shadow test, return 1 value
-- Expect return 3

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function()
		_do._return(3)
	end)
end):emit()
