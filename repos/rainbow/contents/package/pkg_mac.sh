#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/HOWLER
mkdir -p Release/HOWLER
cp -R build/Release/HOWLER.app Release/HOWLER/HOWLER.app
cp package/readme.txt Release/HOWLER
pushd Release
rm -f HOWLER_mac.zip
zip -r HOWLER_mac.zip HOWLER
popd
