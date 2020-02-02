-- Test shadow language if statement + type inference

-- Arg: 1
-- Arg: 2
-- Arg: 3
-- Build expect failure

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.printf = _fn(_type.int32, {_type.int8.ptr, _type.vararg})

	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function(argc,argv)
		_var.three = 3
		_do._if(_var.three,
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
