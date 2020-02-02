#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/LD23
mkdir -p Release/LD23
cp -R build/Release/LD23.app Release/LD23/LD23.app
cp package/readme.txt Release/LD23
pushd Release
rm -f LD23_mac.zip
zip -r LD23_mac.zip LD23
popd
