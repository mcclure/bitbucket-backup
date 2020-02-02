-- Test shadow language function calls + type inference (custom function)
-- Build expect failure

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.identity = _fn(_type.idk, {_type.idk}, function(x)
		_do._return(x)
	end)

	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function(argc,argv)
		_var.x = _fn.identity("a string")
		_do._return(_var.x + 1) -- Adding string to number
	end)
end):emit()
