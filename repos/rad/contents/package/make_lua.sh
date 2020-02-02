#!/bin/bash -e

# Run from project root

mkdir -p lua/API/Project
mkdir -p lua/Include
mkdir -p lua/Source
python ../Polycode/Bindings/Scripts/create_lua_library/create_lua_library.py lua_visible Project Polycode.h project Project ./lua/API ./lua/API/Project ./lua/Include ./lua/Source
pushd lua/API
cp ../../source/project_util.lua .
rm -f project.pak
zip -r project.pak *
popd
mv lua/API/project.pak .
pushd Polycode/Framework/Bindings/Lua/Modules/3DPhysics/API
rm -f physics3d.pak
zip -r physics3d.pak *
popd
mv Polycode/Framework/Bindings/Lua/Modules/3DPhysics/API/physics3d.pak .
