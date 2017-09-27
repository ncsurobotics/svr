cmake_minimum_required (VERSION 3.1)
project( svr )
add_subdirectory( lib )
# Delete the find_package bits when moving to opencv2
find_package( PkgConfig REQUIRED )
pkg_check_modules( OPENCV REQUIRED opencv<2 )
message(STATUS LIBS ${OPENCV_VERSION})


# add_subdirectory( lib python server test util )
