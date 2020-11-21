function(createTest NAME)
	set(SAMPLE_NAME test-${NAME})
	add_executable(${SAMPLE_NAME} src/${NAME}.cpp)

	target_link_libraries(${SAMPLE_NAME} Engine)

	set_target_properties(${SAMPLE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/Engine)

	add_test(NAME ${SAMPLE_NAME} COMMAND $<TARGET_FILE:${SAMPLE_NAME}>)

endfunction()

createTest(dummy)
