project(helmake VERSION "0.1.1")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

message(STATUS "Found Flex ${FLEX_VERSION}")
message(STATUS "Found Bison ${BISON_VERSION}")

set(SOURCE_FILES main.cpp)
find_package(Boost 1.55 REQUIRED COMPONENTS program_options filesystem system)

add_executable(helmake ${SOURCE_FILES} ${GENERATED_SOURCE_FILES})
install(TARGETS helmake DESTINATION bin)