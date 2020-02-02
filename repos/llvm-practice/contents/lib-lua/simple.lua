require("config")
require("lib-ffi/import")
require("lib-lua/import")

LLVMInitializeAll()

int1Type = llvm.int1Type()
int8Type = llvm.int8Type()
int32Type = llvm.int32Type()
charPtrType = LLVM.LLVMPointerType(int8Type, 0)  -- address space 0 (arbitrary)
