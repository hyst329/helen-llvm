project(helen_lib)

file(GLOB HelstdFiles core.f4i io.f4i)
file(GLOB HelstdDirs core io)

add_custom_target(helen_lib_copy ALL)
foreach(HelstdFile ${HelstdFiles})
  add_custom_command(TARGET helen_lib_copy POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy 
                     ${HelstdFile} ${CMAKE_BINARY_DIR}/include/.)
endforeach()

foreach(HelstdDir ${HelstdDirs})
  get_filename_component(DIR ${HelstdDir} NAME)
  add_custom_command(TARGET helen_lib_copy POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_directory 
                     ${HelstdDir} ${CMAKE_BINARY_DIR}/include/${DIR})
endforeach()
set(HelstdSrcFiles core/string.f4)
set(HelstdObjFiles "")
foreach(HelstdSrcFile ${HelstdSrcFiles})
  get_filename_component(DIR ${HelstdSrcFile} DIRECTORY)

  add_custom_command(TARGET helen_lib_copy POST_BUILD
                     COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/helen_compiler
                     --include-path=${CMAKE_BINARY_DIR}/include
                     --include-path=${CMAKE_BINARY_DIR}/include/${DIR}
                     ${CMAKE_BINARY_DIR}/include/${HelstdSrcFile})
  add_custom_command(TARGET helen_lib_copy POST_BUILD
                     COMMAND llc ${CMAKE_BINARY_DIR}/include/${HelstdSrcFile}.bc -filetype=obj)
  set(HelstdObjFiles "${HelstdObjFiles} \"${CMAKE_BINARY_DIR}/include/${HelstdSrcFile}.obj\"")  
endforeach()
make_directory(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
add_custom_command(TARGET helen_lib_copy POST_BUILD
                   COMMAND ar rcs "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libhelstd.a" ${HelstdObjFiles})