#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/Universe
mkdir -p Release/Universe
cp -R build/Release/Universe.app Release/Universe/Universe.app
cp package/readme.txt Release/Universe
pushd Release
rm -f Universe_mac.zip
zip -r Universe_mac.zip Universe
popd
