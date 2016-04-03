project(helc VERSION "0.1.3")

configure_file(${PROJECT_SOURCE_DIR}/cmake/config.h.cmake ${PROJECT_SOURCE_DIR}/cmake/config.h)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

message(STATUS "Found Flex ${FLEX_VERSION}")
message(STATUS "Found Bison ${BISON_VERSION}")

find_package(LLVM REQUIRED CONFIG)
find_package(Boost 1.55 REQUIRED COMPONENTS filesystem system)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

BISON_TARGET(HelenParser grammar/helen.y
             ${PROJECT_SOURCE_DIR}/grammar/helen.parser.cpp COMPILE_FLAGS "--verbose")
FLEX_TARGET(HelenScanner grammar/helen.l
            ${PROJECT_SOURCE_DIR}/grammar/helen.lexer.cpp)
ADD_FLEX_BISON_DEPENDENCY(HelenScanner HelenParser)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -static-libgcc -static-libstdc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -lpthread")

set(SOURCE_FILES main.cpp
                 AST.cpp AST.h
                 Error.cpp Error.h
                 FunctionNameMangler.cpp FunctionNameMangler.h
                 BuiltinFunctions.cpp BuiltinFunctions.h)
set(GENERATED_SOURCE_FILES ${BISON_HelenParser_OUTPUTS} ${FLEX_HelenScanner_OUTPUTS})
add_executable(helc ${SOURCE_FILES} ${GENERATED_SOURCE_FILES})

llvm_map_components_to_libnames(llvm_libs support core irreader scalaropts bitwriter ipo)

target_link_libraries(helc ${Boost_LIBRARIES} ${llvm_libs})

install(TARGETS helc DESTINATION bin)