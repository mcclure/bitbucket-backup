namespace "namespace1"

-- This file is in namespace1, so it will be able to see aglobal

return {
	value = function() return aglobal end
}
