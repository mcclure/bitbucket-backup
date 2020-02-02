# You don't get a "good" script, I'm on an LD timeframe.
curl -O http://curl.haxx.se/download/curl-7.27.0.tar.gz
tar zxvf curl-7.27.0.tar.gz
mkdir curl-product
mkdir curl-product-win
mkdir curl-product-lin

# After this, go inside curl-7.2.7.0 folder and say ./configure --prefix=$PWD/../curl-product; make install
# Then you have to go in and delete the .la and .dylib, for SOME STUPID REASON.
# Maybe this could be avoided by telling ./configure to build static only?
# --enable-shared=no worked on Linux
# Windows, I built by doing this:
# cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Windows" -DCMAKE_TOOLCHAIN_FILE=/Users/mcc/work/p/tmp/toolchain.cmake -DCMAKE_INSTALL_PREFIX:PATH=$PWD/../curl-product-win -DCURL_STATICLIB=on -DBUILD_CURL_EXE=off -DBUILD_CURL_TESTS=off 
