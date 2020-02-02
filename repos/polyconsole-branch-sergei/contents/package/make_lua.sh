#!/bin/bash -e

# Run from project root

if [[ ! -e block_lua ]]; then # To temporarily suppress this script create this file

mkdir -p lua/API/Project
mkdir -p lua/Include
mkdir -p lua/Source

${PYTHON:-python} Polycode-extras/create_lua_library.py lua_visible.txt Project Polycode.h project Project ./lua/API ./lua/API/Project ./lua/Include ./lua/Source ./media/project_classes.txt
pushd lua/API
cp ../../source/project_util.lua .
rm -f project.pak
zip -r project.pak *
popd
mv lua/API/project.pak .

pushd Polycode/Framework/Bindings/Lua/Modules/2DPhysics/API
rm -f physics2d.pak
zip -r physics2d.pak *
popd
mv Polycode/Framework/Bindings/Lua/Modules/2DPhysics/API/physics2d.pak .

pushd Polycode/Framework/Bindings/Lua/Modules/3DPhysics/API
rm -f physics3d.pak
zip -r physics3d.pak *
popd
mv Polycode/Framework/Bindings/Lua/Modules/3DPhysics/API/physics3d.pak .

fi
