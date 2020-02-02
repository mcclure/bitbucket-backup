#!/bin/bash -e

cd ../..  
./dopack.sh # FIXME: Should this be called from somewhere else?
./package/make_lua.sh 
