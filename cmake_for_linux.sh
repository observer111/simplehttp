#!/bin/bash

./cleancmake.sh


export LD_LIBRARY_PATH="`pwd`/linux/simplehttp:$LD_LIBRARY_PATH"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"

BUILD_DIR=linux-build

rm -rf $BUILD_DIR
mkdir $BUILD_DIR
cd $BUILD_DIR
cmake \
    -D CMAKE_TOOLCHAIN_FILE=../toolchain-generic-linux.cmake \
    ..

make $* && ( echo;echo "made: "
    ls -l --color=always `pwd`/simplehttp/libsimplehttp.so
    file `pwd`/simplehttp/libsimplehttp.so
    ls -l  --color=always  `pwd`/test/test_simplehttp
    file `pwd`/test/test_simplehttp

echo;echo "# to run it:"
echo -ne "\033[31;42;1m"
echo -n "LD_LIBRARY_PATH=\"./linux-build/simplehttp:\$LD_LIBRARY_PATH\" ./linux-build/test/test_simplehttp"
echo -ne "\033[0m"
echo
)
