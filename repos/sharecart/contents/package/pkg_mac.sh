#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/Player
mkdir -p Release/Player
mkdir -p Release/dat
cp -R build/Release/Player.app Release/Player/Player.app
cp package/readme.txt Release/Player
pushd Release
rm -f Player_mac.zip
zip -r Player_mac.zip Player dat
popd
