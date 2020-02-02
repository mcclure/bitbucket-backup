#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/Diabolus
mkdir -p Release/Diabolus
cp -R build/Release/Diabolus.app Release/Diabolus/Diabolus.app
cp package/readme.txt Release/Diabolus
pushd Release
rm -f Diabolus_mac.zip
zip -r Diabolus_mac.zip Diabolus
popd
