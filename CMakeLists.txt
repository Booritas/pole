cmake_minimum_required(VERSION 3.1.2)
project(pole)

IF(WIN32)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
ENDIF(WIN32)

option(PACKAGE_TESTS "Build the tests" ON)
if(PACKAGE_TESTS)
        enable_testing()
        include(GoogleTest)
        add_subdirectory(tests)
endif()
 
#include(conanbuildinfo.cmake)
#conan_basic_setup()

add_subdirectory(includes)
add_subdirectory(sources)

add_library(${PROJECT_NAME} ${SOURCE_FILES})

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
