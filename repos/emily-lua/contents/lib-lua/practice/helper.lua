-- Little FFI/LLVM helpers

-- Convert LLVM Message to lua string and dispose
function takeMessage(cstr)
	local str = ffi.string(cstr)
	LLVM.LLVMDisposeMessage(cstr)
	return str
end

local errPtr = ffi.new("char *[1]")
function llvmErr(context)
	error((context or "Unknown") .. " error: " .. takeMessage(errPtr[0]))
end

-- Convert lua string to garbage-collected c string
function cString(str)
	local cstr = ffi.new(string.format("char[%d]", #str))
	ffi.copy(cstr, str)
	return cstr
end

function cArray(type, values)
	local arrayType = string.format("%s [%d]", type, #values)
	local array = ffi.new(arrayType)
	for i,v in ipairs(values) do
		array[i-1] = v
	end
	return array
end

typeArray = func.bind1(cArray, "LLVMTypeRef")
valueArray = func.bind1(cArray, "LLVMValueRef")

-- Misc helpers

function printerr(message) -- Print to stderr
	io.stderr:write(message .. "\n")
end

function toboolean(x)
	return x and true or false
end

function capitalize(str)
	return str:gsub("^%l", string.upper)
end

function endswith(str, ending)
	if not str then return false end
	return ending == '' or string.sub(str, -string.len(ending)) == ending
end