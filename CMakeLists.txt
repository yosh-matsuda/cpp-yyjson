cmake_minimum_required(VERSION 3.16)
project(cpp_yyjson VERSION 0.0.1 LANGUAGES CXX)

# check if the project is top-level
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CPPYYJSON_IS_TOPLEVEL_PROJECT ON)
else()
    set(CPPYYJSON_IS_TOPLEVEL_PROJECT OFF)
endif()

# options
option(CPPYYJSON_BUILD_TEST "Build ${PROJECT_NAME} tests" ${CPPYYJSON_IS_TOPLEVEL_PROJECT})
option(CPPYYJSON_BUILD_BENCH "Build ${PROJECT_NAME} tests and benchmarks" OFF)

# library
include(GNUInstallDirs)
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

# dependencies
find_package(yyjson CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} INTERFACE yyjson::yyjson)
find_package(visit_struct CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} INTERFACE visit_struct::visit_struct)
find_package(magic_enum CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} INTERFACE magic_enum::magic_enum)
find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} INTERFACE fmt::fmt-header-only)
find_package(nameof CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} INTERFACE nameof::nameof)

# install
if(CPPYYJSON_IS_TOPLEVEL_PROJECT)
    install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_Targets)

    install(
        EXPORT ${PROJECT_NAME}_Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)
    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

    # install config files
    install(
        FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

    # install header files
    install(
        DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

# test and benchmark
if(CPPYYJSON_BUILD_TEST OR CPPYYJSON_BUILD_BENCH)
    enable_testing()
    add_subdirectory(test)
endif()
