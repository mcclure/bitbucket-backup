#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/Flipper
mkdir -p Release/Flipper
cp -R build/Release/Flipper.app Release/Flipper/Flipper.app
cp package/readme.txt Release/Flipper
pushd Release
rm -f Flipper_mac.zip
zip -r Flipper_mac.zip Flipper
popd
