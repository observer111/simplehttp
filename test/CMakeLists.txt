
include_directories( ../simplehttp )

add_executable (test_simplehttp test_simplehttp.cpp)

if (WINDOWS)
    add_definitions(${CMAKE_EXE_LINKER_FLAGS}
         "-static -static-libgcc -static-libstdc++")
endif()

find_library(simplehttp
        NAME simplehttp
        PATHS
     ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_SYSTEM_NAME}/
     ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_SYSTEM_NAME}/simplehttp
     ${CMAKE_SYSTEM_NAME}/simplehttp
        DOC "simplehttp library"
)

target_link_libraries (test_simplehttp LINK_PUBLIC simplehttp pthread ${CMAKE_EXE_LINKER_LIBS} )

