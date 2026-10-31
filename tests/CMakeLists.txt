set(TEST_SOURCES
    test_md5.cpp
    test_sha1.cpp
    test_sha256.cpp
    test_thread_pool.cpp
    test_file_scanner.cpp
    test_comparator.cpp
    test_integration.cpp
)

foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
    target_link_libraries(${TEST_NAME} PRIVATE hashchecker_core)
    add_sanitizers(${TEST_NAME})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
