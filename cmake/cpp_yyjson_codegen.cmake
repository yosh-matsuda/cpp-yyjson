# cpp_yyjson_generate(): C++ types from a JSON Schema with cpp-yyjson-codegen, at build time.
# This file is part of cpp-yyjson (https://github.com/yosh-matsuda/cpp-yyjson), MIT License.
#
#   cpp_yyjson_generate(<target> <schema>
#                       [OUTPUT <header>] [PUBLIC | PRIVATE | INTERFACE]
#                       [NAMESPACE <ns>] [ROOT_NAME <name>] [MAX_ALTERNATIVES <n>]
#                       [CPP26] [NO_PATTERN] [OPTIONAL_DEFAULTS] [KEEP_UNKNOWN_KEYS] [STRICT]
#                       [DEPENDS <file>...])
#
# Generates <header> from <schema> and adds it to the sources of <target>, so that the target is
# built after the header, and adds the directory of the header to the include directories of the
# target with the given scope (PRIVATE by default). A relative <schema> is taken from the current
# source directory, a relative <header> from the current binary directory, and the default
# <header> is the file name of the schema up to its first dot, with `.hpp`. The options match those
# of the command line. Files that the schema references with `$ref` are tracked through a depfile
# where the generator supports one (Ninja, or CMake 3.21 and later); elsewhere, list them in DEPENDS.
# Call it in the directory that defines <target>: CMake before 3.20 does not see a generated file in
# the sources of a target of another directory. An INTERFACE library needs CMake 3.19 or later.
#
# The generator is the target `cpp_yyjson::cpp_yyjson_codegen`, which CPPYYJSON_BUILD_CODEGEN=ON
# builds and installs, or the program that CPPYYJSON_CODEGEN_EXECUTABLE names, for example one built
# for the host when cross-compiling.

include_guard(GLOBAL)

set(CPPYYJSON_CODEGEN_EXECUTABLE
    ""
    CACHE FILEPATH "cpp-yyjson-codegen to run in place of the target cpp_yyjson::cpp_yyjson_codegen")

# A function runs with the policies in effect where it is defined. CMP0116 makes CMake take the
# paths of a depfile under Ninja as they are, and omits the warning that it is not set.
cmake_policy(PUSH)
if(POLICY CMP0116)
  cmake_policy(SET CMP0116 NEW)
endif()

function(cpp_yyjson_generate target schema)
  cmake_parse_arguments(
    PARSE_ARGV 2 arg
    "PUBLIC;PRIVATE;INTERFACE;CPP26;NO_PATTERN;OPTIONAL_DEFAULTS;KEEP_UNKNOWN_KEYS;STRICT"
    "OUTPUT;NAMESPACE;ROOT_NAME;MAX_ALTERNATIVES" "DEPENDS")
  if(arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "cpp_yyjson_generate: unknown arguments: ${arg_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT TARGET ${target})
    message(FATAL_ERROR "cpp_yyjson_generate: target '${target}' does not exist")
  endif()

  set(scope PRIVATE)
  set(scopes 0)
  foreach(s PUBLIC PRIVATE INTERFACE)
    if(arg_${s})
      set(scope ${s})
      math(EXPR scopes "${scopes} + 1")
    endif()
  endforeach()
  if(scopes GREATER 1)
    message(FATAL_ERROR "cpp_yyjson_generate: give one of PUBLIC, PRIVATE and INTERFACE")
  endif()

  if(CPPYYJSON_CODEGEN_EXECUTABLE)
    set(generator "${CPPYYJSON_CODEGEN_EXECUTABLE}")
    set(generator_target "")
  elseif(TARGET cpp_yyjson::cpp_yyjson_codegen)
    get_target_property(generator_target cpp_yyjson::cpp_yyjson_codegen ALIASED_TARGET)
    if(NOT generator_target)
      set(generator_target cpp_yyjson::cpp_yyjson_codegen)
    endif()
    set(generator "$<TARGET_FILE:${generator_target}>")
  else()
    message(
      FATAL_ERROR
        "cpp_yyjson_generate: cpp-yyjson-codegen is not available. Build cpp-yyjson with "
        "CPPYYJSON_BUILD_CODEGEN=ON, or set CPPYYJSON_CODEGEN_EXECUTABLE to the program.")
  endif()

  get_filename_component(schema "${schema}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  if(arg_OUTPUT)
    get_filename_component(output "${arg_OUTPUT}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  else()
    get_filename_component(stem "${schema}" NAME)
    string(REGEX REPLACE "\\..*$" "" stem "${stem}")
    set(output "${CMAKE_CURRENT_BINARY_DIR}/${stem}.hpp")
  endif()
  get_filename_component(output_dir "${output}" DIRECTORY)

  set(options "")
  if(arg_NAMESPACE)
    list(APPEND options --namespace "${arg_NAMESPACE}")
  endif()
  if(arg_ROOT_NAME)
    list(APPEND options --root-name "${arg_ROOT_NAME}")
  endif()
  if(arg_MAX_ALTERNATIVES)
    list(APPEND options --max-alternatives "${arg_MAX_ALTERNATIVES}")
  endif()
  foreach(flag CPP26 NO_PATTERN OPTIONAL_DEFAULTS KEEP_UNKNOWN_KEYS STRICT)
    if(arg_${flag})
      string(TOLOWER "--${flag}" option)
      string(REPLACE "_" "-" option "${option}")
      list(APPEND options "${option}")
    endif()
  endforeach()

  set(depfile_arguments "")
  if(CMAKE_GENERATOR MATCHES "Ninja" OR NOT CMAKE_VERSION VERSION_LESS 3.21)
    set(depfile "${output}.d")
    set(depfile_arguments DEPFILE "${depfile}")
    list(APPEND options --depfile "${depfile}")
  endif()

  file(RELATIVE_PATH shown "${CMAKE_BINARY_DIR}" "${output}")
  add_custom_command(
    OUTPUT "${output}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_dir}"
    COMMAND "${generator}" "${schema}" -o "${output}" ${options}
    DEPENDS "${schema}" ${generator_target} ${arg_DEPENDS}
    ${depfile_arguments}
    COMMENT "Generating ${shown} from a JSON Schema"
    VERBATIM)

  target_sources(${target} PRIVATE "${output}")
  target_include_directories(${target} ${scope} "$<BUILD_INTERFACE:${output_dir}>")
endfunction()

cmake_policy(POP)
