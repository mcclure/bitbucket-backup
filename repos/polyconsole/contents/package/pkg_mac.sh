#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/Polyethylene
mkdir -p Release/Polyethylene
cp -R build/Release/Polyethylene.app Release/Polyethylene/Polyethylene.app
cp package/readme.txt Release/Polyethylene
pushd Release
rm -f Polyethylene_mac.zip
zip -r Polyethylene_mac.zip Polyethylene
popd
