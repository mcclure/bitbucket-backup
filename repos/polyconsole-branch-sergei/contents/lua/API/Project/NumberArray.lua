class "NumberArray"







function NumberArray:NumberArray(...)
	for k,v in pairs(arg) do
		if type(v) == "table" then
			if v.__ptr ~= nil then
				arg[k] = v.__ptr
			end
		end
	end
	if self.__ptr == nil and arg[1] ~= "__skip_ptr__" then
		self.__ptr = Project.NumberArray(unpack(arg))
		__ptr_lookup.NumberArray[self.__ptr] = self
	end
end

function NumberArray:size()
	local retVal =  Project.NumberArray_size(self.__ptr)
	return retVal
end

function NumberArray:get(at)
	local retVal = Project.NumberArray_get(self.__ptr, at)
	return retVal
end

function NumberArray:push_back(value)
	local retVal = Project.NumberArray_push_back(self.__ptr, value)
end



if not __ptr_lookup then __ptr_lookup = {} end
__ptr_lookup.NumberArray = {}

function NumberArray:__delete()
	__ptr_lookup.NumberArray[self.__ptr] = nil
	Project.delete_NumberArray(self.__ptr)
end
