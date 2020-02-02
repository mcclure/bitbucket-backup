require "Polycode/EventHandler"

class "automaton" (EventHandler)



function automaton:__index__(name)
	if name == "done" then
		return Project.automaton_get_done(self.__ptr)
	elseif name == "born" then
		return Project.automaton_get_born(self.__ptr)
	elseif name == "frame" then
		return Project.automaton_get_frame(self.__ptr)
	elseif name == "state" then
		return Project.automaton_get_state(self.__ptr)
	elseif name == "rollover" then
		return Project.automaton_get_rollover(self.__ptr)
	elseif name == "anchor" then
		retVal = Project.automaton_get_anchor(self.__ptr)
		if __ptr_lookup["auto_iter"][retVal] ~= nil then
			return __ptr_lookup["auto_iter"][retVal]
		else
			__ptr_lookup["auto_iter"][retVal] = _G["auto_iter"]("__skip_ptr__")
			__ptr_lookup["auto_iter"][retVal].__ptr = retVal
			return __ptr_lookup["auto_iter"][retVal]
		end
	end
end


function automaton:__set_callback(name,value)
	if name == "done" then
		Project.automaton_set_done(self.__ptr, value)
		return true
	elseif name == "born" then
		Project.automaton_set_born(self.__ptr, value)
		return true
	elseif name == "frame" then
		Project.automaton_set_frame(self.__ptr, value)
		return true
	elseif name == "state" then
		Project.automaton_set_state(self.__ptr, value)
		return true
	elseif name == "rollover" then
		Project.automaton_set_rollover(self.__ptr, value)
		return true
	end
	return false
end


function automaton:automaton(...)
	if type(arg[1]) == "table" and count(arg) == 1 then
		if ""..arg[1]:class() == "EventHandler" then
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
		self.__ptr = Project.automaton(unpack(arg))
		__ptr_lookup.automaton[self.__ptr] = self
	end
end

function automaton:age()
	local retVal =  Project.automaton_age(self.__ptr)
	if retVal == nil then return nil end
	if __ptr_lookup["int"][retVal] ~= nil then
		return __ptr_lookup["int"][retVal]
	else
		__ptr_lookup["int"][retVal] = _G["int"]("__skip_ptr__")
		__ptr_lookup["int"][retVal].__ptr = retVal
		return __ptr_lookup["int"][retVal]
	end
end

function automaton:tick()
	local retVal =  Project.automaton_tick(self.__ptr)
end

function automaton:die()
	local retVal =  Project.automaton_die(self.__ptr)
end

function automaton:insert()
	local retVal =  Project.automaton_insert(self.__ptr)
end



if not __ptr_lookup then __ptr_lookup = {} end
__ptr_lookup.automaton = {}

function automaton:__delete()
	__ptr_lookup.automaton[self.__ptr] = nil
	Project.delete_automaton(self.__ptr)
end
