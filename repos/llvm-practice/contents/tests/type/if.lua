-- Test shadow language if statement + type inference

-- (By placing this arg here, I am implicitly testing the shared-state feature of regression.py. I am sneaky.)
-- Arg: 1
-- Arg: 2
-- Arg: 3
-- Expect: Argc 4 greater than or equal to 3

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.printf = _fn(_type.int32, {_type.int8.ptr, _type.vararg})

	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function(argc,argv)
		_var.lessThan3 = _get._lt(argc, 3)
		_do._if(_var.lessThan3,
			function()
				_do._fn.printf("Argc %d less than 3\n", argc)
			end,
			function()
				_do._fn.printf("Argc %d greater or equal to 3\n", argc)
			end)
		_do._fn.printf("Done\n")
		_do._return(0)
	end)
end):emit()
