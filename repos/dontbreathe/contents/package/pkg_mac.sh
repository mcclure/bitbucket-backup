#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/Sun\ Sets
mkdir -p Release/Sun\ Sets
cp -R build/Release/Sun_Sets.app Release/Sun\ Sets/Sun\ Sets.app
cp package/readme.txt Release/Sun\ Sets
pushd Release
rm -f Sun_Sets_mac.zip
zip -r Sun_Sets_mac.zip Sun\ Sets
popd
