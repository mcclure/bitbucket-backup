-- Test shadow language function parameters + type inference
-- Arg: 111
-- Arg: 222
-- Arg: 333
-- Expect return 12

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.main = _fn(_type.int32, {_type.idk, _type.int8.ptr.ptr}, function(argc,argv)
		_var.x = argc
		_var.y = _var.x + 4
		_do._return(_var.x + _var.y)
	end)
end):emit()
