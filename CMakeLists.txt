cmake_minimum_required (VERSION 3.1)
project( svr )

# Global Configuration
if( ${CMAKE_SYSTEM_NAME} EQUAL "Linux" )
  set( CMAKE_C_FLAGS "-D__SVR_Linux__" )
endif()

# Subdirectories to be built
add_subdirectory( lib )
add_subdirectory( server )
add_subdirectory( python )
add_subdirectory( util )
add_subdirectory( test )
