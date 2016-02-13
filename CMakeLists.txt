cmake_minimum_required(VERSION 3.0)
project(helen VERSION 0.1.0)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(helen-compiler)
add_subdirectory(helen-ide)
add_subdirectory(helen-lib)

SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_NSIS_MODIFY_PATH ON)
include(CPack)