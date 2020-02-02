class "StringArray"







function StringArray:size()
	local retVal =  Project.StringArray_size(self.__ptr)
	return retVal
end

function StringArray:get(at)
	local retVal = Project.StringArray_get(self.__ptr, at)
	return retVal
end

function StringArray:push_back(value)
	local retVal = Project.StringArray_push_back(self.__ptr, value)
end



if not __ptr_lookup then __ptr_lookup = {} end
__ptr_lookup.StringArray = {}

function StringArray:__delete()
	__ptr_lookup.StringArray[self.__ptr] = nil
	Project.delete_StringArray(self.__ptr)
end
