#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/Xaxxaxoxax
mkdir -p Release/Xaxxaxoxax
cp -R build/Release/Xaxxaxoxax.app Release/Xaxxaxoxax/Xaxxaxoxax.app
cp package/readme.txt Release/Xaxxaxoxax
pushd Release
rm -f Xaxxaxoxax_mac.zip
zip -r Xaxxaxoxax_mac.zip Xaxxaxoxax
popd
