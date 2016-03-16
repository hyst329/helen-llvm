project(helstd)

file(GLOB HelstdFiles core.f4i io.f4i)
file(GLOB HelstdDirs core io)

add_custom_target(helstd_copy ALL)
add_dependencies(helstd_copy helc)
foreach(HelstdFile ${HelstdFiles})
  add_custom_command(TARGET helstd_copy PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy 
                     ${HelstdFile} ${CMAKE_BINARY_DIR}/include/.)
endforeach()

foreach(HelstdDir ${HelstdDirs})
  get_filename_component(DIR ${HelstdDir} NAME)
  add_custom_command(TARGET helstd_copy PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_directory 
                     ${HelstdDir} ${CMAKE_BINARY_DIR}/include/${DIR})
endforeach()
set(HelstdSrcFiles core/string.f4)
set(HelstdObjFiles "")
make_directory(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
make_directory(${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/obj)
foreach(HelstdSrcFile ${HelstdSrcFiles})
  get_filename_component(DIR ${HelstdSrcFile} DIRECTORY)
  get_filename_component(SRC_FN ${HelstdSrcFile} NAME)
  add_custom_command(TARGET helstd_copy PRE_BUILD
                     COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/helen_compiler
                     --include-path=${CMAKE_BINARY_DIR}/include
                     --include-path=${CMAKE_BINARY_DIR}/include/${DIR}
                     ${CMAKE_BINARY_DIR}/include/${HelstdSrcFile})
  add_custom_command(TARGET helstd_copy PRE_BUILD
                     COMMAND llc ${CMAKE_BINARY_DIR}/include/${HelstdSrcFile}.bc -filetype=obj
                     -o ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/obj/${SRC_FN}.o)
  set(HelstdObjFiles "${HelstdObjFiles} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/obj/${SRC_FN}.o")  
endforeach()
string(STRIP ${HelstdObjFiles} HelstdObjFiles)
message("HSOF ${HelstdObjFiles}")
#set(HelstdObjFiles ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/obj/*.o)
message("HSOF ${HelstdObjFiles}")
#add_custom_command(TARGET helen_lib_copy POST_BUILD
#                   COMMAND ar rcs ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libhelstd.a ${HelstdObjFiles})
add_library(helstd STATIC ${HelstdObjFiles})
add_dependencies(helstd helstd_copy)
set_source_files_properties(${HelstdObjFiles} PROPERTIES EXTERNAL_OBJECT true GENERATED true)
set_target_properties(helstd PROPERTIES LINKER_LANGUAGE C
                      ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
                      
install(FILES ${HelstdFiles} DESTINATION include)
install(DIRECTORY ${HelstdDirs} DESTINATION include FILES_MATCHING PATTERN "*.f4i")
install(TARGETS helstd DESTINATION lib)