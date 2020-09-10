set(INCLUDE_ROOT ${PROJECT_SOURCE_DIR}/includes)
set(TEST_NAME storage_tests)

add_subdirectory("${PROJECT_SOURCE_DIR}/googletest" "googletest")

macro(package_add_test TESTNAME)
    # create an exectuable in which the tests will be stored
    add_executable(${TESTNAME} ${ARGN})
    # link the Google test infrastructure, mocking library, and a default main fuction to
    # the test executable. Remove g_test_main if writing your own main function.
    target_link_libraries(${TESTNAME} gtest gmock gtest_main)
    # gtest_discover_tests replaces gtest_add_tests,
    # see https://cmake.org/cmake/help/v3.10/module/GoogleTest.html for more options to pass to it
    gtest_discover_tests(   
        ${TESTNAME}
        # set a working directory so your project root so that you can find test data via paths relative to the project root
        WORKING_DIRECTORY ${PROJECT_DIR}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
    )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

configure_file(test_data.hpp.in ${CMAKE_BINARY_DIR}/test_data.hpp)

package_add_test(${TEST_NAME} 
	test_data.hpp
	stream_utils.hpp
	test_storage.cpp
)
target_include_directories(${TEST_NAME} PRIVATE ${INCLUDE_ROOT} ${CMAKE_BINARY_DIR})
target_link_libraries(${TEST_NAME} ${PROJECT_NAME})
