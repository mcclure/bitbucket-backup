#!/bin/bash -e

if [[ `which python2.7` ]]; then
	export PYTHON=python2.7
fi

cd ../..
./dopack.sh # FIXME: Should this be called from somewhere else?
./package/make_lua.sh 
