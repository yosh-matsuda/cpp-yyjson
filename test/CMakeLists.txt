cmake_minimum_required(VERSION 3.16)

#
# COMPILER/LINKER FLAGS
#
# e.g. -DCMAKE_CXX_STANDARD=26 -DCMAKE_CXX_FLAGS=-freflection for the C++26 reflection backend
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ENV{CMAKE_EXPORT_COMPILE_COMMANDS} ON)

if(NOT MSVC)
  # Warnings
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wcast-align -Wcast-qual -Wdelete-non-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wpointer-arith -Wfloat-equal -Winit-self -Wredundant-decls -Wsign-promo -Wswitch-default -Wswitch-enum -Wvariadic-macros -Wwrite-strings -Wno-unknown-pragmas"
  )

  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")
  elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow=local -Wno-pragmas")
  endif()
else() # MSVC including clang-cl
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /utf-8")
endif()

# O3 optimization
if(NOT MSVC)
  string(REPLACE "-O2" "-O3" CMAKE_CXX_FLAGS_RELWITHDEBINFO
                 ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
  string(REPLACE "-O2" "-O3" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
endif()

# GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# executables
add_executable(
  "${PROJECT_NAME}_test"
  test_dom_write.cpp
  test_dom_read.cpp
  test_readme.cpp
  test_dom_structs.cpp
  test_direct_writer.cpp
  test_direct_reader.cpp
  test_json_view.cpp
  test_conversions.cpp
  test_casters.cpp
  test_read_into.cpp
  test_reflection.cpp
  test_recursive_types.cpp
  test_std_reflection.cpp
  test_differential.cpp
  test_tagged_variant.cpp
  test_error_path.cpp)
target_link_libraries("${PROJECT_NAME}_test"
                      PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)
# a test file can exceed the sections of an object file that MSVC writes by default
target_compile_options("${PROJECT_NAME}_test"
                       PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

# a caster of the user for a type that the library reads by itself applies to the whole program
add_executable("${PROJECT_NAME}_test_user_casters" test_user_casters.cpp)
target_link_libraries("${PROJECT_NAME}_test_user_casters"
                      PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)

# JSONTestSuite, the number tests and generated texts, from the datasets of the benchmark submodule
add_executable("${PROJECT_NAME}_test_conformance" test_conformance.cpp)
target_link_libraries("${PROJECT_NAME}_test_conformance"
                      PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)
target_compile_definitions(
  "${PROJECT_NAME}_test_conformance"
  PRIVATE
    CPPYYJSON_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/external/yyjson_benchmark/data"
)

# the depth limit configures the backend as well, which is compiled here with it
if(CPPYYJSON_USE_BUNDLED_YYJSON)
  add_executable("${PROJECT_NAME}_test_depth_limit" test_depth_limit.cpp
                                                    ../src/yyjson.c)
  target_include_directories("${PROJECT_NAME}_test_depth_limit"
                             SYSTEM PRIVATE ../include)
  target_compile_definitions("${PROJECT_NAME}_test_depth_limit"
                             PRIVATE YYJSON_READER_DEPTH_LIMIT=4)
  target_compile_options(
    "${PROJECT_NAME}_test_depth_limit"
    PRIVATE $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:preprocessor>)
  target_link_libraries("${PROJECT_NAME}_test_depth_limit"
                        PRIVATE GTest::gtest GTest::gtest_main)
endif()

# add google test
include(GoogleTest)
gtest_discover_tests("${PROJECT_NAME}_test" DISCOVERY_MODE PRE_TEST)
gtest_discover_tests("${PROJECT_NAME}_test_user_casters" DISCOVERY_MODE PRE_TEST)
gtest_discover_tests("${PROJECT_NAME}_test_conformance" DISCOVERY_MODE PRE_TEST)
if(CPPYYJSON_USE_BUNDLED_YYJSON)
  gtest_discover_tests("${PROJECT_NAME}_test_depth_limit" DISCOVERY_MODE
                       PRE_TEST)
endif()

# benchmark
if(CPPYYJSON_BUILD_BENCH)
  # Google benchmark
  find_package(benchmark REQUIRED)

  # JSON libraries
  find_package(simdjson CONFIG REQUIRED)
  # simdjson 4.6.4 exports SIMDJSON_DEVELOPMENT_CHECKS for every GCC/Clang build, not
  # only for Debug (fixed upstream after that release), which leaves its API-misuse
  # checks in the inlined On Demand code of the release benchmarks.
  get_target_property(_simdjson_defs simdjson::simdjson
                      INTERFACE_COMPILE_DEFINITIONS)
  if(_simdjson_defs)
    list(REMOVE_ITEM _simdjson_defs SIMDJSON_DEVELOPMENT_CHECKS)
    set_target_properties(simdjson::simdjson PROPERTIES
                          INTERFACE_COMPILE_DEFINITIONS "${_simdjson_defs}")
  endif()
  find_package(RapidJSON CONFIG REQUIRED)
  find_package(nlohmann_json CONFIG REQUIRED)

  # cpp-yyjson comparison benchmarks
  add_executable("${PROJECT_NAME}_bench_read" bench_read.cpp)
  target_link_libraries(
    "${PROJECT_NAME}_bench_read" PRIVATE ${PROJECT_NAME} benchmark::benchmark
    simdjson::simdjson rapidjson)

    add_executable("${PROJECT_NAME}_bench_write" bench_write.cpp)
    target_link_libraries(
      "${PROJECT_NAME}_bench_write"
      PRIVATE ${PROJECT_NAME} benchmark::benchmark rapidjson nlohmann_json
      nlohmann_json::nlohmann_json)

  add_executable("${PROJECT_NAME}_bench_typed" bench_typed.cpp)
  target_link_libraries("${PROJECT_NAME}_bench_typed"
                        PRIVATE ${PROJECT_NAME} benchmark::benchmark)

  # Glaze benchmarks.  Glaze requires C++23, which the rest of the project does
  # not, so its rows are built from the same sources into executables of their
  # own, the way the upstream yyjson rows are.  cpp-yyjson is linked in as the
  # reference the Glaze results are checked against.
  find_package(glaze CONFIG REQUIRED)
  add_executable(glaze_bench_read bench_read.cpp)
  target_link_libraries(
    glaze_bench_read PRIVATE ${PROJECT_NAME} benchmark::benchmark glaze::glaze
                             simdjson::simdjson rapidjson)
  add_executable(glaze_bench_write bench_write.cpp)
  target_link_libraries(
    glaze_bench_write PRIVATE ${PROJECT_NAME} benchmark::benchmark glaze::glaze
                              rapidjson nlohmann_json nlohmann_json::nlohmann_json)
  add_executable(glaze_bench_typed bench_typed.cpp)
  target_link_libraries(
    glaze_bench_typed PRIVATE ${PROJECT_NAME} benchmark::benchmark glaze::glaze)
  foreach(target glaze_bench_read glaze_bench_write glaze_bench_typed)
    target_compile_features(${target} PRIVATE cxx_std_23)
    target_compile_definitions(${target} PRIVATE CPPYYJSON_GLAZE_BENCHMARKS)
  endforeach()

  # Upstream yyjson C API benchmarks
  find_package(yyjson CONFIG REQUIRED)
  add_executable(yyjson_bench_read bench_read.cpp)
  target_link_libraries(
    yyjson_bench_read PRIVATE benchmark::benchmark yyjson::yyjson
                              simdjson::simdjson rapidjson)
  target_compile_features(yyjson_bench_read PRIVATE cxx_std_20)
  target_compile_definitions(yyjson_bench_read PRIVATE CPPYYJSON_RAW_YYJSON_BENCHMARKS)

  add_executable(yyjson_bench_write bench_write.cpp)
  target_link_libraries(yyjson_bench_write PRIVATE benchmark::benchmark yyjson::yyjson)
  target_compile_features(yyjson_bench_write PRIVATE cxx_std_20)
  target_compile_definitions(yyjson_bench_write PRIVATE CPPYYJSON_RAW_YYJSON_BENCHMARKS)

  # reflect-cpp benchmarks.  reflect-cpp reads and writes JSON through the yyjson
  # of vcpkg, which defines the same symbols as the bundled backend, so its
  # executable compiles cpp-yyjson, the reference its results are checked
  # against, on that yyjson as well, as CPPYYJSON_USE_BUNDLED_YYJSON=OFF does.
  find_package(reflectcpp CONFIG REQUIRED)
  add_executable(reflectcpp_bench_typed bench_typed.cpp)
  target_link_libraries(reflectcpp_bench_typed PRIVATE benchmark::benchmark
                                                       reflectcpp::reflectcpp)
  target_compile_features(reflectcpp_bench_typed PRIVATE cxx_std_20)
  target_compile_definitions(reflectcpp_bench_typed PRIVATE CPPYYJSON_REFLECTCPP_BENCHMARKS)

  get_target_property(YYJSON_INCLUDE_DIRECTORIES yyjson::yyjson INTERFACE_INCLUDE_DIRECTORIES)
  foreach(target yyjson_bench_read yyjson_bench_write reflectcpp_bench_typed)
    target_include_directories(
      ${target}
      SYSTEM BEFORE PRIVATE "$<BUILD_INTERFACE:${YYJSON_INCLUDE_DIRECTORIES}>"
                            "${PROJECT_SOURCE_DIR}/include")
  endforeach()

  # The benchmark sources are shared between the cpp-yyjson and the upstream
  # yyjson executables, so a flag given to only one side would land on the
  # measured code itself and show up as a difference between the libraries.
  # LTO is turned on for the yyjson side as well even though the prebuilt yyjson
  # cannot take part in it; both libraries keep the flags they are shipped with,
  # which is the comparison the benchmarks are about.
  foreach(target "${PROJECT_NAME}_bench_read" "${PROJECT_NAME}_bench_write"
                 "${PROJECT_NAME}_bench_typed" yyjson_bench_read yyjson_bench_write
                 glaze_bench_read glaze_bench_write glaze_bench_typed
                 reflectcpp_bench_typed)
    cpp_yyjson_enable_lto(${target})
    if(NOT MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
      target_compile_options(${target} PRIVATE -march=x86-64-v3)
    endif()
  endforeach()
endif()

# compile commands
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_target(gen_compile_commands ALL
                    COMMENT "Generating compile_commands.json")

  # Generate a compilation database with header files
  find_program(COMPDB compdb)

  if(COMPDB)
    add_custom_command(
      TARGET gen_compile_commands
      PRE_BUILD
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMAND ${COMPDB} list > ${PROJECT_SOURCE_DIR}/compile_commands.json
      COMMENT "Running compdb")

  elseif(NOT WIN32)
    message(AUTHOR_WARNING "'compdb' not found")

    # Create symbolic link instead of compdb
    add_custom_command(
      TARGET gen_compile_commands
      PRE_BUILD
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMAND
        ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${PROJECT_SOURCE_DIR}/compile_commands.json
      COMMENT "Creating symbolic link")
  endif()
endif()
