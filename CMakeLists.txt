cmake_minimum_required(VERSION 3.11)
project(helen VERSION 0.2.0)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# External libraries
add_subdirectory(external/libfirm)

add_subdirectory(helc)
add_subdirectory(helstd)
add_subdirectory(helmake)

SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "2")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_VERSION "0.2.0-alpha2")
SET(CPACK_NSIS_MODIFY_PATH ON)

if(UNIX AND NOT APPLE)
  SET(CPACK_GENERATOR "DEB")
  SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Gerry King")
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "llvm (>= 7.0~) libboost-dev (>= 1.68~)")
endif(UNIX AND NOT APPLE)

if(WIN32)
  SET(CPACK_GENERATOR "NSIS")
endif(WIN32)
include(CPack)
