cmake_minimum_required (VERSION 3.1)
project ( check )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPENCV_CFLAGS}") 
include_directories ( ../include/ ${OPENCV_INCLUDE_DIRS} )
list(APPEND LIBRARIES ${OPENCV_LDFLAGS} )
add_executable( check test_source.c )
add_dependencies( check libsvr )
# I cannot get the 
target_link_libraries( check libsvr pthread ${OPENCV_LIBRARIES} )
