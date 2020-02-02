namespace = require "namespace"

local lib1 = require "example.lib1"
local lib2 = require "example.lib2"
local lib3 = require "example.lib3"

print( aglobal, lib1.value(), lib2.value(), lib3.value() )
