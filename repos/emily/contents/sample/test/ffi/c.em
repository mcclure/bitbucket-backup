# Test foreign calls to C functions, arity 1-8
# (Because every arity has a different implementation, each one needs its own test case)

# Expect:
# 6.
# 2.
# 33.
# 27.
# 21.
# 15.
# 14.
# 10.

increment3 = package.emily.ffi.c.function [
    name= .TESTincrement3; args=[.int,]; return= .int
]

subtract = package.emily.ffi.c.function [
    name= .TESTsubtract; args=[.string, .int]; return= .int
]

multThenSubtract = package.emily.ffi.c.function [
    name= .TESTmultThenSubtract; args=[.int, .double, .string]; return= .int
]

multThenSubtractMult = package.emily.ffi.c.function [
    name= .TESTmultThenSubtractMult; args=[.int, .double, .string, .int]; return= .int
]

multSubtractMultSubtract = package.emily.ffi.c.function [
    name= .TESTmultSubtractMultSubtract; args=[.int, .double, .string, .int, .string]; return= .int
]

multSubtractMultSubtractMult = package.emily.ffi.c.function [
    name= .TESTmultSubtractMultSubtractMult; args=[.int, .double, .string, .int, .string, .double]; return= .int
]

multSubtractMultSubtractMultSubtract = package.emily.ffi.c.function [
    name= .TESTmultSubtractMultSubtractMultSubtract; args=[.int, .double, .string, .int, .string, .double, .int]; return= .int
]

multSubtractMultSubtractMultSubtractMult = package.emily.ffi.c.function [
    name= .TESTmultSubtractMultSubtractMultSubtractMult; args=[.int, .double, .string, .int, .string, .double, .int, .double]; return= .int
]
println: increment3 3

println: subtract "7" 5

println: multThenSubtract 7 5 "2"

println: multThenSubtractMult 7 5 "2" 4

println: multSubtractMultSubtract 7 5 "2" 4 "6"

println: multSubtractMultSubtractMult 7 5 "2" 4 "6" 2

println: multSubtractMultSubtractMultSubtract 7 5 "2" 4 "6" 2 1

println: multSubtractMultSubtractMultSubtractMult 7 5 "2" 4 "6" 2 1 5
