cmake_minimum_required (VERSION 3.1)
project ( libsvr )
# set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CV_CFLAGS}" )
include_directories ( ../include/ ${OPENCV_INCLUDE_DIRS})
list(APPEND LIBRARIES ${OPENCV_LDFLAGS})

add_library ( libsvr SHARED blockalloc.c mempool.c message.c pack.c net.c logging.c refcount.c	
  frameproperties.c encoding.c lockable.c main.c encodings/raw.c
  responseset.c messagerouting.c messagehandlers.c stream.c source.c
  comm.c optionstring.c encodings/jpeg.c )
set_target_properties( libsvr PROPERTIES SOVERSION 0)
install( TARGETS libsvr DESTINATION "lib")
target_link_libraries( libsvr pthread seawolf jpeg )
