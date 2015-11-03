project(helen_ide)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

link_directories(${GTKMM_LIBRARY_DIRS})
include_directories(${GTKMM_INCLUDE_DIRS})

add_executable(helen_ide main.cpp)

target_link_libraries(helen_ide ${GTKMM_LIBRARIES})