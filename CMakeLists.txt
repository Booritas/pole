cmake_minimum_required(VERSION 3.1.2)
project(pole)

IF(WIN32)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
ENDIF(WIN32)

set(CMAKE_CXX_STANDARD,14)
IF(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++")
ENDIF()

option(PACKAGE_TESTS "Build the tests" ON)

if(PACKAGE_TESTS)
        enable_testing()
        include(GoogleTest)
        add_subdirectory(tests)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#include(conanbuildinfo.cmake)
#conan_basic_setup()

add_subdirectory(includes)
add_subdirectory(sources)

add_library(${PROJECT_NAME} ${SOURCE_FILES})
