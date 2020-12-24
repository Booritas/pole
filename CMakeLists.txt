cmake_minimum_required(VERSION 3.1.2)
project(pole)


set(CMAKE_CXX_STANDARD,14)

IF(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++")
ELSEIF(UNIX)
    IF((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9"))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
        add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
    ELSE()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libstdc++")
        add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
    ENDIF()
ELSEIF(WIN32)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
ENDIF()

option(PACKAGE_TESTS "Build the tests" ON)

if(PACKAGE_TESTS)
        enable_testing()
        include(GoogleTest)
        add_subdirectory(tests)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/install/lib)

add_subdirectory(includes)
add_subdirectory(sources)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/includes"
        DESTINATION ${CMAKE_BINARY_DIR}/install
        FILES_MATCHING 
                PATTERN "*.hpp"
                PATTERN "*.h"
                PATTERN "*.hh"
        )