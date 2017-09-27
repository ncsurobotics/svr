cmake_minimum_required (VERSION 3.1)
project( svr )
add_subdirectory( lib )
# Delete the find_package bits when moving to opencv2
find_package( PkgConfig REQUIRED )
pkg_check_modules( OpenCV REQUIRED opencv )
set( $ENV{CV_LDFLAGS} "${OpenCV_LDFLAGS} ${OpenCV_LDFLAGS_OTHER}" )
message( "CV_LDFLAGS: ${CV_LDFLAGS}" )
set( $ENV{CV_CFLAGS} "${OpenCV_CFLAGS} ${OpenCV_CFLAGS_OTHER}" )
message( "CV_CFLAGS: ${CV_CFLAGS}" )

# add_subdirectory( lib python server test util )
