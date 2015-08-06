
# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)
SET(WINDOWS 1)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
SET(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

# TODO make this a cmake/cl option: ?
add_definitions(${CMAKE_CXX_FLAGS} "-static-libgcc -static-libstdc++ -static -Wall -std=c++11 -DUSE_STD_THREAD -D_WIN32 -DBUILDING_DLL -static -Wno-unknown-pragmas")

add_definitions(${CMAKE_MODULE_LINKER_FLAGS} "-static-libgcc -static-libstdc++ -static -shared -lws2_32 -Wl,--out-implib,libexample_dll.a -Wl,-no-undefined -Wl,--enable-runtime-pseudo-reloc -lws2_32 -static")

add_definitions(${CMAKE_EXE_LINKER_FLAGS} "-static-libgcc -static-libstdc++ -static ")
set(CMAKE_EXE_LINKER_LIBS "-lws2_32 -static")

