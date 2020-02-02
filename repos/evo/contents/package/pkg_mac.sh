#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/LD24
mkdir -p Release/LD24
cp -R build/Release/LD24.app Release/LD24/LD24.app
cp package/readme.txt Release/LD24
pushd Release
rm -f LD24_mac.zip
zip -r LD24_mac.zip LD24
popd
