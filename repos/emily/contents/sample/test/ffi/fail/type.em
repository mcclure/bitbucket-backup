# Test foreign calls to C functions (symbol not found)
# Expect failure

increment3 = package.emily.ffi.c.function [
    name= .thisNameDoesNotExist; args=[.int,]; return= .int
]

println: increment3 3
