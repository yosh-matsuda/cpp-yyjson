cmake_minimum_required(VERSION 3.16)

#
# COMPILER/LINKER FLAGS
#
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ENV{CMAKE_EXPORT_COMPILE_COMMANDS} ON)

if(UNIX)
    # Prefer -g3 to -g
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g3")

    # Warnings
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wcast-align -Wcast-qual -Wdelete-non-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wpointer-arith -Wfloat-equal -Winit-self -Wredundant-decls -Wconversion -Wsign-promo -Wswitch-default -Wswitch-enum -Wvariadic-macros -Wwrite-strings -Wno-unknown-pragmas"
    )

    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        set(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -Wshadow"
        )
    elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        set(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -Wshadow=local -Wno-pragmas"
        )
    endif()
else() # WIN32
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# O3 optimization
if(NOT MSVC)
    string(REPLACE "-O2" "-O3" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    string(REPLACE "-O2" "-O3" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
endif()

# GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# executables
add_executable("${PROJECT_NAME}_test" test.cpp)
target_link_libraries("${PROJECT_NAME}_test" PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)

# add google test
include(GoogleTest)
gtest_discover_tests("${PROJECT_NAME}_test")

# benchmark
if(CPPYYJSON_BUILD_BENCH)
    # Google benchmark
    find_package(benchmark REQUIRED)

    # JSON libraries
    find_package(simdjson CONFIG REQUIRED)
    find_package(RapidJSON CONFIG REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)

    # executables
    add_executable("${PROJECT_NAME}_bench_read" bench_read.cpp)
    target_link_libraries("${PROJECT_NAME}_bench_read" PRIVATE ${PROJECT_NAME} benchmark::benchmark simdjson::simdjson rapidjson)
    add_executable("${PROJECT_NAME}_bench_write" bench_write.cpp)
    target_link_libraries("${PROJECT_NAME}_bench_write" PRIVATE ${PROJECT_NAME} benchmark::benchmark rapidjson nlohmann_json nlohmann_json::nlohmann_json)
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
