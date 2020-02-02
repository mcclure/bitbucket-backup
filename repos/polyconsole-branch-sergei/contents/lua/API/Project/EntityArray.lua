class "EntityArray"







function EntityArray:size()
	local retVal =  Project.EntityArray_size(self.__ptr)
	return retVal
end

function EntityArray:get(at)
	local retVal = Project.EntityArray_get(self.__ptr, at)
	if retVal == nil then return nil end
	if __ptr_lookup["Entity"][retVal] ~= nil then
		return __ptr_lookup["Entity"][retVal]
	else
		__ptr_lookup["Entity"][retVal] = _G["Entity"]("__skip_ptr__")
		__ptr_lookup["Entity"][retVal].__ptr = retVal
		return __ptr_lookup["Entity"][retVal]
	end
end

function EntityArray:push_back(value)
	local retVal = Project.EntityArray_push_back(self.__ptr, value.__ptr)
end



if not __ptr_lookup then __ptr_lookup = {} end
__ptr_lookup.EntityArray = {}

function EntityArray:__delete()
	__ptr_lookup.EntityArray[self.__ptr] = nil
	Project.delete_EntityArray(self.__ptr)
end
