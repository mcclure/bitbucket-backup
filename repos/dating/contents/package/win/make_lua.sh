#!/bin/bash -e

# Note: First argument to this script is the path to python, to be passed on.  
if [[ ! -z $1 ]]; then
	export PYTHON=$1
elif [ -e /opt/local/bin/python2.7 ]; then # Will only come up on legacy mac os
	export PYTHON=/opt/local/bin/python2.7
fi

cd ../..  
./dopack.sh # FIXME: Should this be called from somewhere else?
./package/make_lua.sh 
