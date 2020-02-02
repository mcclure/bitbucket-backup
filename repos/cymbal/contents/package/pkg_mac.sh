#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/SuperFungus
mkdir -p Release/SuperFungus
cp -R build/Release/SuperFungus.app Release/SuperFungus/SuperFungus.app
cp package/readme.txt Release/SuperFungus
pushd Release
rm -f SuperFungus_mac.zip
zip -r SuperFungus_mac.zip SuperFungus
popd
