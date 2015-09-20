project(helen_compiler)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

message(STATUS "Found Flex ${FLEX_VERSION}")
message(STATUS "Found Bison ${BISON_VERSION}")

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

BISON_TARGET(HelenParser grammar/helen.y grammar/helen.parser.cpp)
FLEX_TARGET(HelenScanner grammar/helen.l grammar/helen.lexer.cpp)
ADD_FLEX_BISON_DEPENDENCY(HelenScanner HelenParser)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
set(GENERATED_SOURCE_FILES ${BISON_HelenParser_OUTPUTS} ${FLEX_HelenScanner_OUTPUTS})
add_executable(helen_compiler ${SOURCE_FILES} ${GENERATED_SOURCE_FILES})

llvm_map_components_to_libnames(llvm_libs support core irreader)

target_link_libraries(helen_compiler ${llvm_libs})