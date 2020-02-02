#!/bin/bash -e

# Run from project root

./dopack.sh
xcodebuild -configuration Release -project PolycodeTemplate.xcodeproj ARCHS="x86_64"
rm -rf Release/fps2
mkdir -p Release/fps2
cp -R build/Release/fps2.app Release/fps2/fps2.app
cp package/readme.txt Release/fps2
pushd Release
rm -f fps2_mac.zip
zip -r fps2_mac.zip fps2
popd
