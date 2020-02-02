#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/Nothings
mkdir -p Release/Nothings
cp -R build/Release/Nothings.app Release/Nothings/Nothings.app
cp package/readme.txt Release/Nothings
pushd Release
rm -f Nothings_mac.zip
zip -r Nothings_mac.zip Nothings
popd
