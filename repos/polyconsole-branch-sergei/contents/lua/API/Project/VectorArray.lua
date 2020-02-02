class "VectorArray"







function VectorArray:size()
	local retVal =  Project.VectorArray_size(self.__ptr)
	return retVal
end

function VectorArray:get(at)
	local retVal = Project.VectorArray_get(self.__ptr, at)
	if retVal == nil then return nil end
	if __ptr_lookup["Vector3"][retVal] ~= nil then
		return __ptr_lookup["Vector3"][retVal]
	else
		__ptr_lookup["Vector3"][retVal] = _G["Vector3"]("__skip_ptr__")
		__ptr_lookup["Vector3"][retVal].__ptr = retVal
		return __ptr_lookup["Vector3"][retVal]
	end
end

function VectorArray:push_back(value)
	local retVal = Project.VectorArray_push_back(self.__ptr, value.__ptr)
end



if not __ptr_lookup then __ptr_lookup = {} end
__ptr_lookup.VectorArray = {}

function VectorArray:__delete()
	__ptr_lookup.VectorArray[self.__ptr] = nil
	Project.delete_VectorArray(self.__ptr)
end
