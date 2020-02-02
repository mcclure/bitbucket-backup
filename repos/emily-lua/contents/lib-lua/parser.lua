require("lib-lua/simple")

lapp = require("pl/lapp")

function filedump( f )
	return f:read("*a")
end
