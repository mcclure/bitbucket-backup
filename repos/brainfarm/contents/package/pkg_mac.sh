#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/PURPLE
mkdir -p Release/PURPLE
cp -R build/Release/PURPLE.app Release/PURPLE/PURPLE.app
cp package/readme.txt Release/PURPLE
pushd Release
rm -f PURPLE_mac.zip
zip -r PURPLE_mac.zip PURPLE
popd
