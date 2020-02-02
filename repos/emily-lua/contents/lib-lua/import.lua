-- Lets internal references in Penlight work
package.path = package.path .. ";./lib-lua/?.lua"

class = require("pl/class")
func = require("pl/func")
pretty = require("pl/pretty")

-- lpeg is currently used by caller.lua which is needed by ast.lua.
-- if ast.lua could lose that dependency, this require could be moved to parser.lua.
package.path = package.path .. ";./lib-lua/lpeglj/?.lua"
lpeg = require("lpeglj")

-- Project-specific libs
require("practice/helper")
require("practice/wrapper")
require("practice/backend")
require("practice/caller")
