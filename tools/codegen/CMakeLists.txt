add_executable(cpp_yyjson_codegen main.cpp)
add_executable(cpp_yyjson::cpp_yyjson_codegen ALIAS cpp_yyjson_codegen)
target_link_libraries(cpp_yyjson_codegen PRIVATE cpp_yyjson)
target_compile_features(cpp_yyjson_codegen PRIVATE cxx_std_20)
set_target_properties(cpp_yyjson_codegen PROPERTIES OUTPUT_NAME cpp-yyjson-codegen)

# exported with the library, as cpp_yyjson::cpp_yyjson_codegen
if(CPPYYJSON_IS_TOPLEVEL_PROJECT)
  install(
    TARGETS cpp_yyjson_codegen
    EXPORT cpp_yyjson_Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
