-- MAKE SURE YOU RUN THIS FROM THE REPO ROOT, NOT FROM scripts/

require("config")
require("lib-lua/import")
require("lib-ffi/import")

LLVMInitializeAll()

local target = LLVM.LLVMGetFirstTarget()
while target ~= NULL do
	print(ffi.string(LLVM.LLVMGetTargetName(target)))
	target = LLVM.LLVMGetNextTarget(target)
end
