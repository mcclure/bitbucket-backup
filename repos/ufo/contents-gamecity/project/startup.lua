-- Startup gunk

-- Get pl accessible
package.path = package.path .. ";./rootlib/?.lua"

-- Insert a custom print in place of the defaults.lua print, which calls to_string first
pretty = require("pl/pretty")
class = require("pl/class")

raw_print = _G["print"]
local function better_print(str)
	raw_print(pretty.write(str))
end
_G["print"] = better_print

function read_file_txt(path)
    local f = io.open(path, "r")
	if not f then return nil end
    local content = f:read("*all")
    f:close()
    return content
end

function read_file_table(path)
	local result = read_file_txt(path)
	if result then
		result = pretty.read( result )
	end
	return result
end


-- Seed random
math.randomseed( use_seed or os.time() )

-- Load "c part" of project
local sdl = require( "ffi/sdl" ) -- So we can parse project cdefs
project = require("ffi/project")