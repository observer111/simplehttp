#!/bin/bash

./cleancmake.sh

BUILD_DIR=win32-build

rm -rf $BUILD_DIR
mkdir $BUILD_DIR
cd $BUILD_DIR
cmake \
    -D CMAKE_TOOLCHAIN_FILE=../toolchain-i686-w64-mingw32.cmake \
    ..

make $* && ( echo;echo "made:" 
    ls -lt --color=always `pwd`/simplehttp/libsimplehttp.dll
    file `pwd`/simplehttp/libsimplehttp.dll
    ls -lt --color=always `pwd`/test/test_simplehttp.exe
    file `pwd`/test/test_simplehttp.exe

echo;echo "# to run it with wine:"
echo -ne "\033[31;42;1m"
echo -n "WINEPATH=\"Z:\\root\\Development\\libsimplehttp\\win32-build\\simplehttp\"  ~/Development/libsimplehttp/win32-build/test/test_simplehttp.exe"
echo -ne "\033[0m"
echo
)

