-- MAKE SURE YOU RUN THIS FROM THE REPO ROOT, NOT FROM scripts/

require("config")
require("lib-lua/import")
require("lib-ffi/import")

print( ffi.string( LLVM.LLVMGetDefaultTargetTriple() ) )
