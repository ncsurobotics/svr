cmake_minimum_required (VERSION 3.1)
project ( svrd )

find_package( OpenCV REQUIRED )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenCV_CFLAGS} -Wall -Werror -Wno-unused-function" )
include_directories ( ../include/ ./include/ ./include/svrd/ ${OpenCV_INCLUDE_DIRS} )

add_executable( svrd client.c event.c main.c messagehandlers.c messagerouting.c server.c source.c stream.c sources/test.c sources/cam.c sources/file.c sources/v4l.c )
add_dependencies( svrd svr )
install( TARGETS svrd DESTINATION "bin")
target_link_libraries( svrd svr seawolf pthread ${OpenCV_LIBRARIES} )
