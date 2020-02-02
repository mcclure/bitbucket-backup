mkdir _dist
rm _dist/iJumpman_plat_1_1.zip
rm -rf _dist/iJumpman
mkdir _dist/iJumpman
cp -R build/Release-iphoneos/iJumpman.app _dist/iJumpman/iJumpman.app
cp /Volumes/LaCie/work/ipt/Apple/Platformers_beta_dist_1.mobileprovision _dist/iJumpman
echo "Open iTunes, and drag each of these two files onto the word LIBRARY.\n\nThen, plug in your iPhone or iPod Touch to upload Jumpman.\n\nIf it doesn't work, select your device in iTunes, look under the Applications tab, and make sure Application syncing is turned on.\n\n--mcc" >> _dist/iJumpman/README.txt
rm _dist/iJumpman/.DS_Store 
zip -r _dist/iJumpman_plat_x_x.zip _dist/iJumpman
