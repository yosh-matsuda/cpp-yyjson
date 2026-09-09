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

# Compile-time options of the bundled yyjson backend. They also switch the
# declarations in `include/yyjson.h`, so they are defined `PUBLIC` below to keep
# the header and `src/yyjson.c` in sync.
set(CPPYYJSON_READER_DEPTH_LIMIT
    ""
    CACHE STRING
          "Depth limit for reading nested arrays/objects (empty for no limit)")
set(CPPYYJSON_WRITER_DEPTH_LIMIT
    ""
    CACHE STRING
          "Depth limit for writing nested arrays/objects (empty for no limit)")
option(CPPYYJSON_DISABLE_FILE
       "Disable the file and `FILE*` read/write APIs of yyjson" OFF)
option(CPPYYJSON_FREESTANDING
       "Build the bundled yyjson backend without libc" OFF)

set(cppyyjson_backend_options CPPYYJSON_READER_DEPTH_LIMIT
                              CPPYYJSON_WRITER_DEPTH_LIMIT)
list(APPEND cppyyjson_backend_options CPPYYJSON_DISABLE_FILE
     CPPYYJSON_FREESTANDING)
if(NOT CPPYYJSON_USE_BUNDLED_YYJSON)
  foreach(cppyyjson_option IN LISTS cppyyjson_backend_options)
    if(${cppyyjson_option})
      message(
        FATAL_ERROR
          "${cppyyjson_option} configures the bundled yyjson backend and "
          "requires CPPYYJSON_USE_BUNDLED_YYJSON=ON. Configure the external "
          "yyjson with the corresponding YYJSON_* option instead.")
    endif()
  endforeach()
endif()

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

# `YYJSON_FREESTANDING` and `YYJSON_DISABLE_FILE` remove declarations from
# `include/yyjson.h`, and the depth limits are read by `include/cpp_yyjson.hpp`
# through the same header, so these definitions must be visible to the consumer
# as well as to `src/yyjson.c`.
if(CPPYYJSON_USE_BUNDLED_YYJSON)
  if(CPPYYJSON_READER_DEPTH_LIMIT)
    target_compile_definitions(
      ${PROJECT_NAME}
      PUBLIC YYJSON_READER_DEPTH_LIMIT=${CPPYYJSON_READER_DEPTH_LIMIT})
  endif()
  if(CPPYYJSON_WRITER_DEPTH_LIMIT)
    target_compile_definitions(
      ${PROJECT_NAME}
      PUBLIC YYJSON_WRITER_DEPTH_LIMIT=${CPPYYJSON_WRITER_DEPTH_LIMIT})
  endif()
  if(CPPYYJSON_DISABLE_FILE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC YYJSON_DISABLE_FILE=1)
  endif()
  if(CPPYYJSON_FREESTANDING)
    target_compile_definitions(${PROJECT_NAME} PUBLIC YYJSON_FREESTANDING=1)
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

# The LTO archive of the backend holds bitcode only, and a target that does not use LTO cannot
# link it. `-ffat-lto-objects` keeps the machine code beside the bitcode, so the same archive
# serves both kinds of consumer.
if(CPPYYJSON_USE_BUNDLED_YYJSON AND CPPYYJSON_LTO_SUPPORTED)
  include(CheckCCompilerFlag)
  check_c_compiler_flag(-ffat-lto-objects CPPYYJSON_HAS_FAT_LTO_OBJECTS)
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

  if(CPPYYJSON_HAS_FAT_LTO_OBJECTS)
    target_compile_options(
      ${CPPYYJSON_TARGET_NAME}
      PRIVATE $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-ffat-lto-objects>)
  endif()

  # CMake does not put the IPO options on the link line of every language. A compiler such as `nvcc`
  # gives the link to the host compiler, and that rule passes no compile option to it, so the
  # bitcode objects reach a linker that cannot read them. Give each linker language the options that
  # its own link driver takes. The options repeat on a language that already carries them, and a
  # compiler takes the same option twice.
  if(NOT CMAKE_VERSION VERSION_LESS 3.18)
    get_property(cppyyjson_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    foreach(lang IN LISTS cppyyjson_languages)
      set(cppyyjson_ipo_options "")
      if(CMAKE_${lang}_LINK_EXECUTABLE MATCHES "HOST_LINK_LAUNCHER")
        # The host compiler links. The C++ options fit only when it is the C++ compiler itself.
        if(CMAKE_${lang}_HOST_LINK_LAUNCHER STREQUAL CMAKE_CXX_COMPILER)
          set(cppyyjson_ipo_options "${CMAKE_CXX_COMPILE_OPTIONS_IPO}")
        endif()
      elseif(DEFINED CMAKE_${lang}_COMPILE_OPTIONS_IPO)
        set(cppyyjson_ipo_options "${CMAKE_${lang}_COMPILE_OPTIONS_IPO}")
      endif()
      if(cppyyjson_ipo_options)
        target_link_options(
          ${target}
          PRIVATE
          "$<$<AND:$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>,$<LINK_LANGUAGE:${lang}>>:${cppyyjson_ipo_options}>"
        )
      endif()
    endforeach()
  endif()
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
