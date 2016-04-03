project(helmake VERSION "0.1.3")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

message(STATUS "Found Flex ${FLEX_VERSION}")
message(STATUS "Found Bison ${BISON_VERSION}")

set(SOURCE_FILES main.cpp)
find_package(Boost 1.55 REQUIRED COMPONENTS filesystem system)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -static-libgcc -static-libstdc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -lpthread")

BISON_TARGET(HelmakeParser grammar/helmake.y
             ${PROJECT_SOURCE_DIR}/grammar/helmake.parser.cpp COMPILE_FLAGS "--verbose")
FLEX_TARGET(HelmakeScanner grammar/helmake.l
            ${PROJECT_SOURCE_DIR}/grammar/helmake.lexer.cpp)
ADD_FLEX_BISON_DEPENDENCY(HelmakeScanner HelmakeParser)

set(GENERATED_SOURCE_FILES ${BISON_HelmakeParser_OUTPUTS} ${FLEX_HelmakeScanner_OUTPUTS})

add_executable(helmake ${SOURCE_FILES} ${GENERATED_SOURCE_FILES})
target_link_libraries(helmake ${Boost_LIBRARIES})

install(TARGETS helmake DESTINATION bin)