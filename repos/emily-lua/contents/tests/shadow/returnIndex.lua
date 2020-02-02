-- Test shadow language array indexing
-- Arg: 10
-- Expect return 20

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.atoi = _fn(_type.int32, {_type.int8.ptr})

	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function(argc,argv)
		_var.x = _fn.atoi( argv[1] )
		_do._return(_var.x + 10)
	end)
end):emit()
