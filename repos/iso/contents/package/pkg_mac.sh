#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/LD25
mkdir -p Release/LD25
cp -R build/Release/LD25.app Release/LD25/LD25.app
cp package/readme.txt Release/LD25
pushd Release
rm -f LD25_mac.zip
zip -r LD25_mac.zip LD25
popd
