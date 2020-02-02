-- Test shadow language array indexing
-- Arg: 10
-- Expect return 20

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.main = _fn(_type.int32, {_type.int32, _type.idk}, function(argc,argv)
		_do._is(argv, _type.int8.ptr.ptr)
		_var.q = argv
		_do._return(10)
	end)
end):emit()
