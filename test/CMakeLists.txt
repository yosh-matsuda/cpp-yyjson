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

# With GCC, each of the heaviest test files takes 7-9 GB to compile and the others up to 4.5 GB.
# Ninja runs as many jobs as there are cores, which exceeds the memory of a CI runner (16 GB), so
# the test files are compiled in two job pools whose depths follow the physical memory: the
# heaviest files in one, everything else in this directory and the generator in the other.
cmake_host_system_information(RESULT CPPYYJSON_MEMORY_MIB QUERY TOTAL_PHYSICAL_MEMORY)
math(EXPR CPPYYJSON_HEAVY_TEST_JOBS "${CPPYYJSON_MEMORY_MIB} / 24576")
if(CPPYYJSON_HEAVY_TEST_JOBS LESS 1)
  set(CPPYYJSON_HEAVY_TEST_JOBS 1)
endif()
math(EXPR CPPYYJSON_TEST_JOBS
     "(${CPPYYJSON_MEMORY_MIB} - 2048 - 9216 * ${CPPYYJSON_HEAVY_TEST_JOBS}) / 4608")
if(CPPYYJSON_TEST_JOBS LESS 1)
  set(CPPYYJSON_TEST_JOBS 1)
endif()
set_property(
  GLOBAL APPEND
  PROPERTY JOB_POOLS cpp_yyjson_heavy_test=${CPPYYJSON_HEAVY_TEST_JOBS}
           cpp_yyjson_test=${CPPYYJSON_TEST_JOBS})
set(CMAKE_JOB_POOL_COMPILE cpp_yyjson_test)
if(TARGET cpp_yyjson_codegen)
  set_target_properties(cpp_yyjson_codegen PROPERTIES JOB_POOL_COMPILE cpp_yyjson_test)
endif()

add_library("${PROJECT_NAME}_test_heavy" OBJECT test_constraints.cpp
                                                test_many_fields.cpp)
target_link_libraries("${PROJECT_NAME}_test_heavy" PRIVATE ${PROJECT_NAME}
                                                           GTest::gtest)
target_compile_options("${PROJECT_NAME}_test_heavy"
                       PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
set_target_properties("${PROJECT_NAME}_test_heavy"
                      PROPERTIES JOB_POOL_COMPILE cpp_yyjson_heavy_test)

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
  test_error_path.cpp
  test_strict.cpp
  test_transparent.cpp
  test_as_array.cpp
  test_validate.cpp)
target_link_libraries(
  "${PROJECT_NAME}_test" PRIVATE ${PROJECT_NAME} "${PROJECT_NAME}_test_heavy"
                                 GTest::gtest GTest::gtest_main)
# a test file can exceed the sections of an object file that MSVC writes by default
target_compile_options("${PROJECT_NAME}_test"
                       PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

# a caster of the user for a type that the library reads by itself applies to the whole program
add_executable("${PROJECT_NAME}_test_user_casters" test_user_casters.cpp)
target_link_libraries("${PROJECT_NAME}_test_user_casters"
                      PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)

# the schemas of cpp_yyjson_schema.hpp, checked with the JSON Schema validator of jsoncons
find_package(jsoncons CONFIG REQUIRED)
add_executable("${PROJECT_NAME}_test_schema" test_schema.cpp)
target_link_libraries("${PROJECT_NAME}_test_schema"
                      PRIVATE ${PROJECT_NAME} jsoncons::jsoncons GTest::gtest GTest::gtest_main)
target_compile_options("${PROJECT_NAME}_test_schema"
                       PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
set_target_properties("${PROJECT_NAME}_test_schema"
                      PROPERTIES JOB_POOL_COMPILE cpp_yyjson_heavy_test)

# the generator of tools/codegen, tested as a library
add_executable("${PROJECT_NAME}_test_codegen" test_codegen.cpp)
target_include_directories("${PROJECT_NAME}_test_codegen"
                           PRIVATE ${PROJECT_SOURCE_DIR}/tools/codegen)
target_link_libraries("${PROJECT_NAME}_test_codegen"
                      PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)

# types generated at build time by cpp_yyjson_generate(), when the generator is built
if(TARGET cpp_yyjson_codegen)
  add_executable("${PROJECT_NAME}_test_codegen_cmake" test_codegen_cmake.cpp)
  target_link_libraries("${PROJECT_NAME}_test_codegen_cmake"
                        PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/order.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/defaults.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/alternatives.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/patterns.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/recursion.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/untyped.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/conditions.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/nested_conditions.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/condition_groups.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/constants.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/contains.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/negations.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/positions.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/rejections.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/dynamic.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/value_conditions.schema.json
                      NAMESPACE model)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/untyped.schema.json
                      OUTPUT untyped_inferred.hpp NAMESPACE inferred INFER_TYPES)
  cpp_yyjson_generate("${PROJECT_NAME}_test_codegen_cmake" codegen/invoice.schema.json
                      NAMESPACE model SCHEMAS codegen/tax.schema.json
                      URI_MAP https://schemas.example.com/=codegen/remote)
endif()

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
gtest_discover_tests("${PROJECT_NAME}_test_schema" DISCOVERY_MODE PRE_TEST)
gtest_discover_tests("${PROJECT_NAME}_test_codegen" DISCOVERY_MODE PRE_TEST)
if(TARGET cpp_yyjson_codegen)
  gtest_discover_tests("${PROJECT_NAME}_test_codegen_cmake" DISCOVERY_MODE PRE_TEST)
endif()
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
    simdjson::simdjson rapidjson nlohmann_json nlohmann_json::nlohmann_json)

    add_executable("${PROJECT_NAME}_bench_write" bench_write.cpp)
    target_link_libraries(
      "${PROJECT_NAME}_bench_write"
      PRIVATE ${PROJECT_NAME} benchmark::benchmark simdjson::simdjson rapidjson
      nlohmann_json nlohmann_json::nlohmann_json)

  add_executable("${PROJECT_NAME}_bench_typed" bench_typed.cpp)
  target_link_libraries("${PROJECT_NAME}_bench_typed"
                        PRIVATE ${PROJECT_NAME} benchmark::benchmark)

  # Glaze benchmark.  Glaze requires C++23, which the rest of the project does
  # not, so its rows are built from the same source into an executable of its
  # own, the way the upstream yyjson rows are.  cpp-yyjson is linked in as the
  # reference the Glaze results are checked against.
  find_package(glaze CONFIG REQUIRED)
  add_executable(glaze_bench_typed bench_typed.cpp)
  target_link_libraries(
    glaze_bench_typed PRIVATE ${PROJECT_NAME} benchmark::benchmark glaze::glaze)
  target_compile_features(glaze_bench_typed PRIVATE cxx_std_23)
  target_compile_definitions(glaze_bench_typed PRIVATE CPPYYJSON_GLAZE_BENCHMARKS)

  # Upstream yyjson C API benchmarks
  find_package(yyjson CONFIG REQUIRED)
  add_executable(yyjson_bench_read bench_read.cpp)
  target_link_libraries(
    yyjson_bench_read PRIVATE benchmark::benchmark yyjson::yyjson
                              simdjson::simdjson rapidjson nlohmann_json::nlohmann_json)
  target_compile_features(yyjson_bench_read PRIVATE cxx_std_20)
  target_compile_definitions(yyjson_bench_read PRIVATE CPPYYJSON_RAW_YYJSON_BENCHMARKS)

  add_executable(yyjson_bench_write bench_write.cpp)
  target_link_libraries(yyjson_bench_write PRIVATE benchmark::benchmark yyjson::yyjson simdjson::simdjson)
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
                 glaze_bench_typed
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
