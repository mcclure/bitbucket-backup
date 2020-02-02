#!/bin/bash -e

cd ../..
./package/make_doxygen.py
./dopack.sh # FIXME: Should this be called from somewhere else?
./package/make_lua.sh 
