namespace "namespace1"

-- Populate a global variable named "aglobal" into namespace1

aglobal = 3

return {
	value = function() return aglobal end
}
