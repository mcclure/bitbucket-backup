class "project_bridge"







function project_bridge:room_screen()
	local retVal =  Project.project_bridge_room_screen(self.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["Screen"][retVal] ~= nil then
		return __ptr_lookup["Screen"][retVal]
	else
		__ptr_lookup["Screen"][retVal] = _G["Screen"]("__skip_ptr__")
		__ptr_lookup["Screen"][retVal].__ptr = retVal
		return __ptr_lookup["Screen"][retVal]
	end
end

function project_bridge:room_scene()
	local retVal =  Project.project_bridge_room_scene(self.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["Scene"][retVal] ~= nil then
		return __ptr_lookup["Scene"][retVal]
	else
		__ptr_lookup["Scene"][retVal] = _G["Scene"]("__skip_ptr__")
		__ptr_lookup["Scene"][retVal].__ptr = retVal
		return __ptr_lookup["Scene"][retVal]
	end
end

function project_bridge:room_objs()
	local retVal =  Project.project_bridge_room_objs(self.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["StringArray"][retVal] ~= nil then
		return __ptr_lookup["StringArray"][retVal]
	else
		__ptr_lookup["StringArray"][retVal] = _G["StringArray"]("__skip_ptr__")
		__ptr_lookup["StringArray"][retVal].__ptr = retVal
		return __ptr_lookup["StringArray"][retVal]
	end
end

function project_bridge:room_oncollide_objs()
	local retVal =  Project.project_bridge_room_oncollide_objs(self.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["StringArray"][retVal] ~= nil then
		return __ptr_lookup["StringArray"][retVal]
	else
		__ptr_lookup["StringArray"][retVal] = _G["StringArray"]("__skip_ptr__")
		__ptr_lookup["StringArray"][retVal].__ptr = retVal
		return __ptr_lookup["StringArray"][retVal]
	end
end

function project_bridge:room_onclick_objs()
	local retVal =  Project.project_bridge_room_onclick_objs(self.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["StringArray"][retVal] ~= nil then
		return __ptr_lookup["StringArray"][retVal]
	else
		__ptr_lookup["StringArray"][retVal] = _G["StringArray"]("__skip_ptr__")
		__ptr_lookup["StringArray"][retVal].__ptr = retVal
		return __ptr_lookup["StringArray"][retVal]
	end
end

function project_bridge:room_id(_at)
	local retVal = Project.project_bridge_room_id(self.__ptr, _at)
	if retVal == nil then return nil end
	if __ptr_lookup["Entity"][retVal] ~= nil then
		return __ptr_lookup["Entity"][retVal]
	else
		__ptr_lookup["Entity"][retVal] = _G["Entity"]("__skip_ptr__")
		__ptr_lookup["Entity"][retVal].__ptr = retVal
		return __ptr_lookup["Entity"][retVal]
	end
end

function project_bridge:room_name(of)
	local retVal = Project.project_bridge_room_name(self.__ptr, of.__ptr)
	return retVal
end

function project_bridge:room_a()
	local retVal =  Project.project_bridge_room_a(self.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["Entity"][retVal] ~= nil then
		return __ptr_lookup["Entity"][retVal]
	else
		__ptr_lookup["Entity"][retVal] = _G["Entity"]("__skip_ptr__")
		__ptr_lookup["Entity"][retVal].__ptr = retVal
		return __ptr_lookup["Entity"][retVal]
	end
end

function project_bridge:room_b()
	local retVal =  Project.project_bridge_room_b(self.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["Entity"][retVal] ~= nil then
		return __ptr_lookup["Entity"][retVal]
	else
		__ptr_lookup["Entity"][retVal] = _G["Entity"]("__skip_ptr__")
		__ptr_lookup["Entity"][retVal].__ptr = retVal
		return __ptr_lookup["Entity"][retVal]
	end
end

function project_bridge:room_remove_scene(obj)
	local retVal = Project.project_bridge_room_remove_scene(self.__ptr, obj.__ptr)
end

function project_bridge:room_remove_screen(obj, doRemove)
	local retVal = Project.project_bridge_room_remove_screen(self.__ptr, obj.__ptr, doRemove)
end

function project_bridge:load_room(from)
	local retVal = Project.project_bridge_load_room(self.__ptr, from)
end

function project_bridge:load_room_txt(from)
	local retVal = Project.project_bridge_load_room_txt(self.__ptr, from)
end

function project_bridge:standalone_screen(svg, objectPath, isPhysics)
	local retVal = Project.project_bridge_standalone_screen(self.__ptr, svg, objectPath, isPhysics)
	if retVal == nil then return nil end
	if __ptr_lookup["Screen"][retVal] ~= nil then
		return __ptr_lookup["Screen"][retVal]
	else
		__ptr_lookup["Screen"][retVal] = _G["Screen"]("__skip_ptr__")
		__ptr_lookup["Screen"][retVal].__ptr = retVal
		return __ptr_lookup["Screen"][retVal]
	end
end

function project_bridge:room_remove_standalone_screen_all(s)
	local retVal = Project.project_bridge_room_remove_standalone_screen_all(self.__ptr, s.__ptr)
end

function project_bridge:clear()
	local retVal =  Project.project_bridge_clear(self.__ptr)
end

function project_bridge:meshFor(p)
	local retVal = Project.project_bridge_meshFor(self.__ptr, p.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["ScreenMesh"][retVal] ~= nil then
		return __ptr_lookup["ScreenMesh"][retVal]
	else
		__ptr_lookup["ScreenMesh"][retVal] = _G["ScreenMesh"]("__skip_ptr__")
		__ptr_lookup["ScreenMesh"][retVal].__ptr = retVal
		return __ptr_lookup["ScreenMesh"][retVal]
	end
end

function project_bridge:saved_level()
	local retVal =  Project.project_bridge_saved_level(self.__ptr)
	return retVal
end

function project_bridge:set_saved_level(priority)
	local retVal = Project.project_bridge_set_saved_level(self.__ptr, priority)
end

function project_bridge:filedump(_path)
	local retVal = Project.project_bridge_filedump(self.__ptr, _path)
	return retVal
end

function project_bridge:filedump_external(_path)
	local retVal = Project.project_bridge_filedump_external(self.__ptr, _path)
	return retVal
end

function project_bridge:help(_path)
	local retVal = Project.project_bridge_help(self.__ptr, _path)
	return retVal
end

function project_bridge:fake()
	local retVal =  Project.project_bridge_fake(self.__ptr)
end

function project_bridge:Quit()
	local retVal =  Project.project_bridge_Quit(self.__ptr)
end

function project_bridge:mmult(a, b)
	local retVal = Project.project_bridge_mmult(self.__ptr, a.__ptr, b.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["Matrix4"][retVal] ~= nil then
		return __ptr_lookup["Matrix4"][retVal]
	else
		__ptr_lookup["Matrix4"][retVal] = _G["Matrix4"]("__skip_ptr__")
		__ptr_lookup["Matrix4"][retVal].__ptr = retVal
		return __ptr_lookup["Matrix4"][retVal]
	end
end

function project_bridge:qmult(a, b)
	local retVal = Project.project_bridge_qmult(self.__ptr, a.__ptr, b.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["Quaternion"][retVal] ~= nil then
		return __ptr_lookup["Quaternion"][retVal]
	else
		__ptr_lookup["Quaternion"][retVal] = _G["Quaternion"]("__skip_ptr__")
		__ptr_lookup["Quaternion"][retVal].__ptr = retVal
		return __ptr_lookup["Quaternion"][retVal]
	end
end

function project_bridge:Slerp(fT, rkP, rkQ, shortestPath)
	local retVal = Project.project_bridge_Slerp(self.__ptr, fT, rkP.__ptr, rkQ.__ptr, shortestPath)
	if retVal == nil then return nil end
	if __ptr_lookup["Quaternion"][retVal] ~= nil then
		return __ptr_lookup["Quaternion"][retVal]
	else
		__ptr_lookup["Quaternion"][retVal] = _G["Quaternion"]("__skip_ptr__")
		__ptr_lookup["Quaternion"][retVal].__ptr = retVal
		return __ptr_lookup["Quaternion"][retVal]
	end
end

function project_bridge:Squad(fT, rkP, rkA, rkB, rkQ, shortestPath)
	local retVal = Project.project_bridge_Squad(self.__ptr, fT, rkP.__ptr, rkA.__ptr, rkB.__ptr, rkQ.__ptr, shortestPath)
	if retVal == nil then return nil end
	if __ptr_lookup["Quaternion"][retVal] ~= nil then
		return __ptr_lookup["Quaternion"][retVal]
	else
		__ptr_lookup["Quaternion"][retVal] = _G["Quaternion"]("__skip_ptr__")
		__ptr_lookup["Quaternion"][retVal].__ptr = retVal
		return __ptr_lookup["Quaternion"][retVal]
	end
end

function project_bridge:bBox(e)
	local retVal = Project.project_bridge_bBox(self.__ptr, e.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["Vector3"][retVal] ~= nil then
		return __ptr_lookup["Vector3"][retVal]
	else
		__ptr_lookup["Vector3"][retVal] = _G["Vector3"]("__skip_ptr__")
		__ptr_lookup["Vector3"][retVal].__ptr = retVal
		return __ptr_lookup["Vector3"][retVal]
	end
end

function project_bridge:setSceneClearColor(scene, r, g, b, a)
	local retVal = Project.project_bridge_setSceneClearColor(self.__ptr, scene.__ptr, r, g, b, a)
end

function project_bridge:project_bridge(...)
	for k,v in pairs(arg) do
		if type(v) == "table" then
			if v.__ptr ~= nil then
				arg[k] = v.__ptr
			end
		end
	end
	if self.__ptr == nil and arg[1] ~= "__skip_ptr__" then
		self.__ptr = Project.project_bridge(unpack(arg))
		__ptr_lookup.project_bridge[self.__ptr] = self
	end
end

function project_bridge:custEntityType(obj)
	local retVal = Project.project_bridge_custEntityType(self.__ptr, obj.__ptr)
	return retVal
end

function project_bridge:charCode(e)
	local retVal = Project.project_bridge_charCode(self.__ptr, e.__ptr)
	return retVal
end

function project_bridge:soundFromValues(values, channels, freq, bps)
	local retVal = Project.project_bridge_soundFromValues(self.__ptr, values.__ptr, channels, freq, bps)
	if retVal == nil then return nil end
	if __ptr_lookup["Sound"][retVal] ~= nil then
		return __ptr_lookup["Sound"][retVal]
	else
		__ptr_lookup["Sound"][retVal] = _G["Sound"]("__skip_ptr__")
		__ptr_lookup["Sound"][retVal].__ptr = retVal
		return __ptr_lookup["Sound"][retVal]
	end
end

function project_bridge:playback_index()
	local retVal =  Project.project_bridge_playback_index(self.__ptr)
end

function project_bridge:playback_from(idx)
	local retVal = Project.project_bridge_playback_from(self.__ptr, idx)
end

function project_bridge:luaTestAddOne(...)
	return Project.project_bridge_luaTestAddOne(self.__ptr, ...)
end
function project_bridge:saveTableIntoObject(...)
	return Project.project_bridge_saveTableIntoObject(self.__ptr, ...)
end
function project_bridge:loadTableFromObject(...)
	return Project.project_bridge_loadTableFromObject(self.__ptr, ...)
end
function project_bridge:saveTableIntoFile(...)
	return Project.project_bridge_saveTableIntoFile(self.__ptr, ...)
end
function project_bridge:loadTableFromFile(...)
	return Project.project_bridge_loadTableFromFile(self.__ptr, ...)
end
function project_bridge:saveTableIntoXml(...)
	return Project.project_bridge_saveTableIntoXml(self.__ptr, ...)
end
function project_bridge:loadTableFromXml(...)
	return Project.project_bridge_loadTableFromXml(self.__ptr, ...)
end
function project_bridge:getChildAtScreenPosition(...)
	return Project.project_bridge_getChildAtScreenPosition(self.__ptr, ...)
end
function project_bridge:getVisible(e)
	local retVal = Project.project_bridge_getVisible(self.__ptr, e.__ptr)
	return retVal
end

function project_bridge:setVisible(e, visible)
	local retVal = Project.project_bridge_setVisible(self.__ptr, e.__ptr, visible)
end

function project_bridge:coreServices()
	local retVal =  Project.project_bridge_coreServices(self.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["CoreServices"][retVal] ~= nil then
		return __ptr_lookup["CoreServices"][retVal]
	else
		__ptr_lookup["CoreServices"][retVal] = _G["CoreServices"]("__skip_ptr__")
		__ptr_lookup["CoreServices"][retVal].__ptr = retVal
		return __ptr_lookup["CoreServices"][retVal]
	end
end



if not __ptr_lookup then __ptr_lookup = {} end
__ptr_lookup.project_bridge = {}

function project_bridge:__delete()
	__ptr_lookup.project_bridge[self.__ptr] = nil
	Project.delete_project_bridge(self.__ptr)
end
