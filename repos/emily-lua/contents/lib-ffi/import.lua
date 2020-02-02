ffi = require( "ffi" )

-- Libraries

LLVM = ffi.load( config.llvmLibPath or "LLVM" )

-- Headers

local llvmDir = "lib-ffi/llvm/" .. config.llvmVersion .. "/"

require(llvmDir .. "Types")
require(llvmDir .. "Target")            -- Requires Types
require(llvmDir .. "TargetMachine")     -- Requires Types, Target
require(llvmDir .. "ExecutionEngine")   -- Requires Types, Target, TargetMachine
require(llvmDir .. "ErrorHandling")
require(llvmDir .. "Core")              -- Requires ErrorHandling, Types
require(llvmDir .. "Analysis")          -- Requires Types
require(llvmDir .. "Transforms/Scalar") -- Requires Types
