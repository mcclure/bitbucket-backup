To run

    cp config.sample.lua config.lua
    (export LLVM_VERSION=3_8; export LLVM_INCLUDE=`llvm-config-3.8 --includedir`; gcc -I$LLVM_INCLUDE -E -P -xc-header ./lib-ffi/llvm/$LLVM_VERSION/Target.source.lua > ./lib-ffi/llvm/$LLVM_VERSION/Target.lua)
    luajit generate.lua
    cc main.o -o main

Then

    ./main

For more information see [https://bitbucket.org/runhello/llvm-practice](https://bitbucket.org/runhello/llvm-practice). Some subdirectories also contain their own README or LICENSE files.

Created by Andi McClure <<andi.m.mcclure@gmail.com>>
