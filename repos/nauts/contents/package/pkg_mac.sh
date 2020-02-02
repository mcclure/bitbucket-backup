#!/bin/bash -e

# Run from project root

./package/make_doxygen.py
./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
mkdir -p Release/Luanauts
cp -R build/Release/Luanauts.app Release/Luanauts/Luanauts.app
cp "package/Lua Tutorial.webloc" "package/Polycode API Reference.webloc" package/readme.txt Release/Luanauts
pushd Release
zip -r Luanauts_mac.zip Luanauts
popd
