# Test foreign calls to C functions (bad argument type during call)
# Note we do not test the case of the function *specification* having a bad type;
# this is because that case probably just crashes! :/

# Expect failure

increment3 = package.emily.ffi.c.function [
    name= .TESTincrement3; args=[.int,]; return= .int
]

println: increment3 "3"
