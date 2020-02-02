namespace "namespace2"

-- This file is not in namespace1, so aglobal will be nil

return {
	value = function() return aglobal end
}
