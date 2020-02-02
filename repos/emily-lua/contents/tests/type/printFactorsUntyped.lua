-- Take one number from input, print its factors as space-separated list (shadow language, all types inferred)
-- Combined test of arithmetic, array indices, branches, functions, C library prototypes
-- This one also tests bool and string variables, so the output has commas...

-- Arg: 4158
-- Expect: 2, 3, 7, 9, 11

require("lib-lua/simple")
require("lib-lua/practice/shadow")

shadowModule("main", function()
	_fn.printf = _fn(_type.int32, {_type.idk, _type.vararg})
	
	-- TODO: make argument idk once pointers work
	_fn.atoi = _fn(_type.idk, {_type.int8.ptr})
	
	_fn.printFactors = _fn(nil, {_type.idk}, function(input)
		_var.value = input
		_var.divisor = 2
		_var.firstPrint = true
		_do._while( _get._gte(_var.value, _var.divisor), function()
			_do._if( _get._eq(_var.value % _var.divisor, 0),
				function()
					_do._if( _var.firstPrint,
						function()
							_var.firstPrint = false
							_var.format = "%d"
						end,
						function()
							_var.format = ", %d"
						end)
					_do._fn.printf(_var.format, _var.divisor)
					_var.value = _var.value / _var.divisor
				end,
				function()
					_var.divisor = _var.divisor + 1
				end)
		end)
		_do._fn.printf("\n")
	end)

	_fn.main = _fn(_type.idk, {_type.idk, _type.idk}, function(argc, argv)
		_do._if( _get._eq(argc, 2),
			function()
				_do._fn.printFactors( _fn.atoi( argv[1] ) )
				_do._return(0)
			end,
			function()
				_do._fn.printf("Please enter exactly one number as argument\n")
				_do._return(1)
			end)
	end)
end):emit()
