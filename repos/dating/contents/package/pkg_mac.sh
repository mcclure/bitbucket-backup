#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/HNSU
mkdir -p Release/HNSU
cp -R build/Release/HNSU.app Release/HNSU/HNSU.app
cp package/readme.txt Release/HNSU
pushd Release
rm -f HNSU_mac.zip
zip -r HNSU_mac.zip HNSU
popd
