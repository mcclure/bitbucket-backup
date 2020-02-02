#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/Fall
mkdir -p Release/Fall
cp -R build/Release/Fall.app Release/Fall/Fall.app
cp package/readme.txt Release/Fall
pushd Release
rm -f Fall_mac.zip
zip -r Fall_mac.zip Fall
popd
