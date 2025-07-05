set(TEST_SOURCES
        test_main.cpp
        from_empty.cpp
)

foreach (TEST_SRC ${TEST_SOURCES})
    get_filename_component(TEST_NAME_BASE "${TEST_SRC}" NAME_WE)

    set(EXECUTABLE_NAME "konfig_${TEST_NAME_BASE}")

    add_executable(${EXECUTABLE_NAME} ${TEST_SRC}
            test_utils.h)

    target_link_libraries(${EXECUTABLE_NAME} konfig)

    string(TOUPPER "${TEST_NAME_BASE}" CAPITALIZED_TEST_NAME)
    set(TEST_DISPLAY_NAME "Konfig${CAPITALIZED_TEST_NAME}")

    add_test(
            NAME ${TEST_DISPLAY_NAME}
            COMMAND ${EXECUTABLE_NAME}
    )

    message(STATUS "Added test: ${TEST_DISPLAY_NAME} (Executable: ${EXECUTABLE_NAME})")
endforeach ()
