cmake_minimum_required (VERSION 3.1)
project ( check )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPENCV_CFLAGS}") 
include_directories ( ../include/ ${OPENCV_INCLUDE_DIRS} )
list(APPEND LIBRARIES ${OPENCV_LDFLAGS} )
add_executable( check test_source.c )
add_dependencies( check svr )
# I cannot get the test.c to build
target_link_libraries( check svr pthread ${OPENCV_LIBRARIES} )
