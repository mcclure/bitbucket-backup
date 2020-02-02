#!/bin/bash -e

rm -f media/.DS_Store
rm -f media.pak
zip -r media.pak media

pushd sub/cs1
./dopack.sh
popd
cp sub/cs1/media.pak ./cs1.pak

pushd sub/diabolus
./dopack.sh
popd
cp sub/diabolus/media.pak ./diabolus.pak

pushd sub/sunsets
./dopack.sh
popd
cp sub/sunsets/media.pak ./sunsets.pak

pushd sub/rainbow
./dopack.sh
popd
cp sub/rainbow/media.pak ./rainbow.pak
