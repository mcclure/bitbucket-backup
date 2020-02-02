#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/ADP
mkdir -p Release/ADP
cp -R build/Release/ADP.app Release/ADP/ADP.app
cp package/readme.txt Release/ADP
pushd Release
rm -f ADP_mac.zip
zip -r ADP_mac.zip ADP
popd
