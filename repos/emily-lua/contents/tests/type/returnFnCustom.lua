-- Test shadow language function calls + type inference (custom function)
-- Expect return 6

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.addtwo = _fn(_type.idk, {_type.idk}, function(x)
		_do._return(x + 2)
	end)

	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function(argc,argv)
		_var.x = _fn.addtwo(3)
		_do._return(_var.x + 1)
	end)
end):emit()
