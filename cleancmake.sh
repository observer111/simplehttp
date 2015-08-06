#!/bin/bash

if [ -e Makefile ] ; then
    make clean
fi

function clean_cmake_tmpfiles {
    echo -n clean: $1
    ( cd $1 ; rm -rf CMakeFiles/ cmake_install.cmake  Makefile \
		  CMakeCache.txt install_manifest.txt )
    echo
}

clean_cmake_tmpfiles simplehttp
clean_cmake_tmpfiles test
clean_cmake_tmpfiles .

clean_cmake_tmpfiles linux-build
clean_cmake_tmpfiles linux-build/simplehttp
clean_cmake_tmpfiles linux-build/test

clean_cmake_tmpfiles win32-build
clean_cmake_tmpfiles win32-build/simplehttp
clean_cmake_tmpfiles win32-build/test

rm -f core
#rm -f tags
