project(helen_ide)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

link_directories(${GTKMM_LIBRARY_DIRS})
include_directories(${GTKMM_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -static-libgcc -static-libstdc++")

set(SOURCE_FILES main.cpp)
file(COPY "ui/temp.glade" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_executable(helen_ide ${SOURCE_FILES})

target_link_libraries(helen_ide ${GTKMM_LIBRARIES})