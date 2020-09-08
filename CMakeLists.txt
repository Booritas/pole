cmake_minimum_required(VERSION 3.1.2)
project(pole)

#include(conanbuildinfo.cmake)
#conan_basic_setup()

add_subdirectory(includes)
add_subdirectory(sources)

add_library(${PROJECT_NAME} ${SOURCE_FILES})

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
