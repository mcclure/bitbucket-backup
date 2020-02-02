#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/LSMQ
mkdir -p Release/LSMQ
cp -R build/Release/LSMQ.app Release/LSMQ/LSMQ.app
cp package/readme.txt Release/LSMQ
pushd Release
rm -f LSMQ_mac.zip
zip -r LSMQ_mac.zip LSMQ
popd
