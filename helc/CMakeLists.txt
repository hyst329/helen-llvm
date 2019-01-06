project(helc VERSION "0.2.0")

configure_file(${PROJECT_SOURCE_DIR}/cmake/config.h.cmake ${PROJECT_SOURCE_DIR}/cmake/config.h)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

message(STATUS "Found Flex ${FLEX_VERSION}")
message(STATUS "Found Bison ${BISON_VERSION}")

find_package(Boost 1.68 REQUIRED COMPONENTS filesystem system)
find_package(Threads REQUIRED)

BISON_TARGET(HelenParser grammar/helen.y
             ${PROJECT_SOURCE_DIR}/grammar/helen.parser.cpp COMPILE_FLAGS "--verbose")
FLEX_TARGET(HelenScanner grammar/helen.l
            ${PROJECT_SOURCE_DIR}/grammar/helen.lexer.cpp)
ADD_FLEX_BISON_DEPENDENCY(HelenScanner HelenParser)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories(${libfirm_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

set(SOURCE_FILES main.cpp
                 AST.cpp AST.h
                 Error.cpp Error.h
                 FunctionNameMangler.cpp FunctionNameMangler.h
                 BuiltinFunctions.cpp BuiltinFunctions.h
                 Generics.cpp Generics.h)
set(GENERATED_SOURCE_FILES ${BISON_HelenParser_OUTPUTS} ${FLEX_HelenScanner_OUTPUTS})
add_executable(helc ${SOURCE_FILES} ${GENERATED_SOURCE_FILES})


target_link_libraries(helc ${Boost_LIBRARIES} firm ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS helc DESTINATION bin)
