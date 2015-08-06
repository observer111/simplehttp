
add_library (simplehttp SHARED SimpleHttp.cpp)

if (WINDOWS)
    target_link_libraries( simplehttp ws2_32 )
    set(CMAKE_SHARED_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
endif()

target_include_directories (simplehttp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

INSTALL(FILES SimpleHttp.hpp DESTINATION include)
INSTALL(TARGETS simplehttp DESTINATION lib)
