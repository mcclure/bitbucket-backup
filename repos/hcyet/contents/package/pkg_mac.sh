#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/HCYET
mkdir -p Release/HCYET
cp -R build/Release/HCYET.app Release/HCYET/HCYET.app
cp package/readme.txt Release/HCYET
pushd Release
rm -f HCYET_mac.zip
zip -r HCYET_mac.zip HCYET
popd
