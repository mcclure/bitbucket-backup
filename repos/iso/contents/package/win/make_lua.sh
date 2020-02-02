#!/bin/bash -e

# Note: First argument to this script is the path to python, to be passed on.

cd ../..  
./dopack.sh # FIXME: Should this be called from somewhere else?
PYTHON=$1 ./package/make_lua.sh 
