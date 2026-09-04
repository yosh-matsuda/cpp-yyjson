cmake_minimum_required(VERSION 3.16)
project(
  cpp_yyjson
  VERSION 0.8.0
  LANGUAGES C CXX)

# check if the project is top-level
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(CPPYYJSON_IS_TOPLEVEL_PROJECT ON)
else()
  set(CPPYYJSON_IS_TOPLEVEL_PROJECT OFF)
endif()

# options
option(CPPYYJSON_BUILD_TEST "Build ${PROJECT_NAME} tests"
       ${CPPYYJSON_IS_TOPLEVEL_PROJECT})
option(CPPYYJSON_BUILD_BENCH "Build ${PROJECT_NAME} tests and benchmarks" OFF)
option(CPPYYJSON_USE_BUNDLED_YYJSON "Build with the bundled SIMD yyjson" ON)

# Benchmarks are meant to measure the fastest available backend.
if(CPPYYJSON_BUILD_BENCH AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
  set(CPPYYJSON_ENABLE_AVX2_DEFAULT ON)
else()
  set(CPPYYJSON_ENABLE_AVX2_DEFAULT OFF)
endif()
option(CPPYYJSON_ENABLE_AVX2
       "Compile the bundled yyjson backend with AVX2 (requires an AVX2 CPU)"
       ${CPPYYJSON_ENABLE_AVX2_DEFAULT})

# library
include(GNUInstallDirs)
if(CPPYYJSON_USE_BUNDLED_YYJSON)
  add_library(${PROJECT_NAME} STATIC src/yyjson.c)
  target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

else()
  add_library(${PROJECT_NAME} INTERFACE)
  target_include_directories(
    ${PROJECT_NAME}
    SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                     $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
endif()
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

target_compile_options(
  ${PROJECT_NAME} INTERFACE $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:preprocessor>)

# The AVX2 paths of the bundled backend are selected by `__AVX2__` at compile
# time, so the flag must reach `src/yyjson.c` itself.
if(CPPYYJSON_USE_BUNDLED_YYJSON AND CPPYYJSON_ENABLE_AVX2)
  if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /arch:AVX2)
  else()
    target_compile_options(${PROJECT_NAME} PRIVATE -mavx2)
  endif()
endif()

# The target name and the LTO support are recorded here, because the body of a function runs in
# the scope of its caller. `${PROJECT_NAME}` in the body names the project of the consumer, and
# `check_ipo_supported` reads variables that CMake sets only in the directory where a language is
# enabled. A consumer whose `project()` call omits `C` would otherwise get "CMake doesn't support
# IPO for current C compiler", although this directory enables `C` for the bundled backend.
set(CPPYYJSON_TARGET_NAME
    ${PROJECT_NAME}
    CACHE INTERNAL "The library target of cpp-yyjson")

if(CPPYYJSON_USE_BUNDLED_YYJSON)
  set(cppyyjson_lto_languages C CXX)
else()
  set(cppyyjson_lto_languages CXX)
endif()
if(NOT DEFINED CPPYYJSON_LTO_LANGUAGES OR NOT CPPYYJSON_LTO_LANGUAGES STREQUAL
                                          "${cppyyjson_lto_languages}")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT cppyyjson_lto_supported OUTPUT cppyyjson_lto_error
                      LANGUAGES ${cppyyjson_lto_languages})
  set(CPPYYJSON_LTO_SUPPORTED
      ${cppyyjson_lto_supported}
      CACHE INTERNAL "Whether LTO is available")
  set(CPPYYJSON_LTO_ERROR
      "${cppyyjson_lto_error}"
      CACHE INTERNAL "Why LTO is not available")
  set(CPPYYJSON_LTO_LANGUAGES
      "${cppyyjson_lto_languages}"
      CACHE INTERNAL "The languages that the LTO check covered")
endif()

function(cpp_yyjson_enable_lto target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "Target '${target}' does not exist")
  endif()

  if(NOT CPPYYJSON_LTO_SUPPORTED)
    message(WARNING "LTO is not supported: ${CPPYYJSON_LTO_ERROR}")
    return()
  endif()

  set(lto_targets ${target})
  if(CPPYYJSON_USE_BUNDLED_YYJSON)
    list(APPEND lto_targets ${CPPYYJSON_TARGET_NAME})
  endif()
  foreach(lto_target ${lto_targets})
    set_property(TARGET ${lto_target} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET ${lto_target} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
  endforeach()
endfunction()

if(NOT CPPYYJSON_USE_BUNDLED_YYJSON)
  function(find_and_link package_name qualified_name)
    cmake_parse_arguments(FIND_AND_LINK "" "VERSION" "" ${ARGN})
    if(NOT TARGET ${package_name} OR CPPYYJSON_IS_TOPLEVEL_PROJECT)
      find_package(${package_name} ${FIND_AND_LINK_VERSION} CONFIG REQUIRED)
      target_link_libraries(${PROJECT_NAME} INTERFACE ${qualified_name})
    else()
      target_link_libraries(${PROJECT_NAME} INTERFACE ${package_name})
    endif()
  endfunction()

  find_and_link(yyjson yyjson::yyjson)
  get_target_property(YYJSON_INCLUDE_DIRECTORIES yyjson::yyjson INTERFACE_INCLUDE_DIRECTORIES)
  target_include_directories(
    ${PROJECT_NAME}
    BEFORE INTERFACE "$<BUILD_INTERFACE:${YYJSON_INCLUDE_DIRECTORIES}>")
endif()

# install
if(CPPYYJSON_IS_TOPLEVEL_PROJECT)
  install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(
    EXPORT ${PROJECT_NAME}_Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    ${PROJECT_NAME}ConfigVersion.cmake COMPATIBILITY SameMajorVersion
                                                     ARCH_INDEPENDENT)
  configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

  # install config files
  install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

  # install header files
  if(CPPYYJSON_USE_BUNDLED_YYJSON)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  else()
    install(FILES ${PROJECT_SOURCE_DIR}/include/cpp_yyjson.hpp
                  ${PROJECT_SOURCE_DIR}/include/field_reflection.hpp
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  endif()
endif()

# test and benchmark
if(CPPYYJSON_BUILD_TEST OR CPPYYJSON_BUILD_BENCH)
  enable_testing()
  add_subdirectory(test)
endif()
