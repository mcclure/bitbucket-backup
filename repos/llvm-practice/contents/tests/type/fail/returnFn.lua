-- Test shadow language function calls + type inference
-- Build expect failure

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.atoi = _fn(_type.idk, {_type.idk})

	_fn.main = _fn(_type.idk, {_type.int32, _type.int8.ptr.ptr}, function(argc,argv)
		_var.x = _fn.atoi("4")
		_do._return(_var.x) -- No information to deduce this type
	end)
end):emit()
