#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/AAACCCuuubbbeeeFFFooorrrYYYooouuu
mkdir -p Release/AAACCCuuubbbeeeFFFooorrrYYYooouuu
cp -R build/Release/AAACCCuuubbbeeeFFFooorrrYYYooouuu.app Release/AAACCCuuubbbeeeFFFooorrrYYYooouuu/AAACCCuuubbbeeeFFFooorrrYYYooouuu.app
cp package/readme.txt Release/AAACCCuuubbbeeeFFFooorrrYYYooouuu
pushd Release
rm -f AAACCCuuubbbeeeFFFooorrrYYYooouuu_mac.zip
zip -r AAACCCuuubbbeeeFFFooorrrYYYooouuu_mac.zip AAACCCuuubbbeeeFFFooorrrYYYooouuu
popd
