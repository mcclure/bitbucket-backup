require "Polycode/SceneEntity"

class "SceneEcho" (SceneEntity)







function SceneEcho:SceneEcho(...)
	if type(arg[1]) == "table" and count(arg) == 1 then
		if ""..arg[1]:class() == "SceneEntity" then
			self.__ptr = arg[1].__ptr
			return
		end
	end
	for k,v in pairs(arg) do
		if type(v) == "table" then
			if v.__ptr ~= nil then
				arg[k] = v.__ptr
			end
		end
	end
	if self.__ptr == nil and arg[1] ~= "__skip_ptr__" then
		self.__ptr = Project.SceneEcho(unpack(arg))
		__ptr_lookup.SceneEcho[self.__ptr] = self
	end
end

function SceneEcho:transformAndRender()
	local retVal =  Project.SceneEcho_transformAndRender(self.__ptr)
end

function SceneEcho:getEntity()
	local retVal =  Project.SceneEcho_getEntity(self.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["Entity"][retVal] ~= nil then
		return __ptr_lookup["Entity"][retVal]
	else
		__ptr_lookup["Entity"][retVal] = _G["Entity"]("__skip_ptr__")
		__ptr_lookup["Entity"][retVal].__ptr = retVal
		return __ptr_lookup["Entity"][retVal]
	end
end



if not __ptr_lookup then __ptr_lookup = {} end
__ptr_lookup.SceneEcho = {}

function SceneEcho:__delete()
	__ptr_lookup.SceneEcho[self.__ptr] = nil
	Project.delete_SceneEcho(self.__ptr)
end
