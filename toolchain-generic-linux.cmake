
# TODO make this a cmake/cl option: ?
# -DUSE_STD_THREAD # :to get a pthread-ed server; but defaults to a forking server
add_definitions(${CMAKE_CXX_FLAGS} "-g")
add_definitions(${CMAKE_CXX_FLAGS} "-Wall")
add_definitions(${CMAKE_CXX_FLAGS} "-std=c++11")
add_definitions(${CMAKE_CXX_FLAGS} "-DUSE_STD_THREAD")
