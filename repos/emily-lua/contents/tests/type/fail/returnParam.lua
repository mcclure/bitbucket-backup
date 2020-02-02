-- Test shadow language function parameters + type inference
-- Arg: 111
-- Arg: 222
-- Build expect failure

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.main = _fn(_type.idk, {_type.int32, _type.int8.ptr.ptr}, function(argc,argv)
		_var.x = "a string"
		_var.x = argc
		_do._return(_var.x)
	end)
end):emit()
