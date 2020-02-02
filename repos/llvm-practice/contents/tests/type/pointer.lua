-- Test type unification with pointers -- flowing "downstream"
-- Arg: 15

-- Build expect:
-- function main; i32 (i32, i8**)
--     var d; i8
--     var q; i8
--     var z; i8*

-- Expect return 10

require("lib-lua/simple")
require("lib-lua/practice/shadow")

mod = shadowModule("main", function()
	_fn.main = _fn(_type.int32, {_type.int32, _type.int8.ptr.ptr}, function(argc,argv)
		_var.q = argv[1][0]
		_var.z = argv[1]
		_var.d = _var.z[0]
		_var.q = _var.d
		_do._return(10)
	end)
end)

mod:emit()
print(mod:debugTypeSummary())
