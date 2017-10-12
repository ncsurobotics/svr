cmake_minimum_required (VERSION 3.1)
project( svr )

# Global Configuration
if( ${CMAKE_SYSTEM_NAME} EQUAL "Linux" )
  set( CMAKE_C_FLAGS "-D__SVR_Linux__" )
elseif( ${CMAKE_SYSTEM_NAME} EQUAL "Darwin" )
  set( CMAKE_C_FLAGS "-D__SW_Darwin__" )
endif()

# Subdirectories to be built
add_subdirectory( lib )
add_subdirectory( server )
add_subdirectory( python )
add_subdirectory( util )
add_subdirectory( test )

# Add uninstall target, taken from the cmake wiki:
# https://cmake.org/Wiki/CMake_FAQ#Can_I_do_.22make_uninstall.22_with_CMake.3F
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()

# Finally the documentation target, which expects to be built from the toplevel dir
add_custom_target(doc COMMAND doxygen ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile)
