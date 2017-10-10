cmake_minimum_required (VERSION 3.1)
project( svr )
add_subdirectory( lib )
add_subdirectory( server )
add_subdirectory( python )

# Delete the find_package bits when moving to opencv2
find_package( PkgConfig REQUIRED )
pkg_check_modules( OPENCV REQUIRED opencv )
message(STATUS LIBS ${OPENCV_VERSION} )
message("ldflags: ${OPENCV_LDFLAGS}")

# add_subdirectory( lib python server test util )
