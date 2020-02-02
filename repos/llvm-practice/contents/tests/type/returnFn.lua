-- Test shadow language function calls
-- Expect return 4

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.atoi = _fn(_type.idk, {_type.idk})

	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function(argc,argv)
		_var.x = _fn.atoi("3")
		_do._return(_var.x + 1)
	end)
end):emit()
