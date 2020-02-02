-- Test type unification with pointers -- flowing "upstream"
-- Arg: 16

-- Build Expect:
-- function main; i32 (i32, i8**)
--     var d; i8
--     var z; i8*
--     var q; i8

-- Expect return 21

require("lib-lua/simple")
require("lib-lua/practice/shadow")

mod = shadowModule("main", function()
	_fn.main = _fn(_type.int32, {_type.int32, _type.idk}, function(argc,argv)
		_var.z = argv[1]
		_var.d = _var.z[0]
		_var.q = _var.d
		_do._is(_var.q, _type.int8)
		_do._return(21)
	end)
end)

mod:emit()
print(mod:debugTypeSummary())
