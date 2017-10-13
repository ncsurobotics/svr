cmake_minimum_required (VERSION 3.1)
project ( check )

find_package( OpenCV REQUIRED )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenCV_CFLAGS}" )
include_directories ( ${OpenCV_INCLUDE_DIRS} ../include )

add_executable( check test_source.c )
add_dependencies( check svr )
# I cannot get the test.c to build
target_link_libraries( check svr pthread ${OpenCV_LIBRARIES} )
