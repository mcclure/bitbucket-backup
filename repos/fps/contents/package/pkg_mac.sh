#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/7DRL
mkdir -p Release/7DRL
cp -R build/Release/7DRL.app Release/7DRL/7DRL.app
cp package/readme.txt Release/7DRL
pushd Release
rm -f 7DRL_mac.zip
zip -r 7DRL_mac.zip 7DRL
popd
