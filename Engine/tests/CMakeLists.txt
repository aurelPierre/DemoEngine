function(createTest NAME)
	set(SAMPLE_NAME test-${NAME})
	add_executable(${SAMPLE_NAME} src/${NAME}.cpp)

	target_link_libraries(${SAMPLE_NAME} Engine)

	add_test(NAME ${SAMPLE_NAME} COMMAND $<TARGET_FILE:${SAMPLE_NAME}>)

endfunction()

createTest(dummy)
