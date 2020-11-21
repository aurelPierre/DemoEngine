cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE SOURCES src/*)

add_executable(Editor ${SOURCES})

target_sources(Editor PUBLIC ${SOURCES})
target_include_directories(Editor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Third Party
target_link_libraries(Editor PUBLIC Engine)

if(CODE_COVERAGE)
else()
	# Warnings
	if(MSVC)
		target_compile_options(Editor PRIVATE /W4)
	else()
		target_compile_options(Editor PRIVATE -Wall -Wextra)
	endif()

	# Multithreaded compilation.
	if(MSVC AND NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") # Not allowed on Clang-cl for Visual Studio.
		target_compile_options(Editor PRIVATE /MP)
	endif()

	# Exceptions handling.
	if(MSVC)
		target_compile_options(Editor PRIVATE /EHsc)
	else()
		target_compile_options(Editor PRIVATE -fexceptions)
	endif()

	# Advanced MSVC preprocessor required for Core::Debug
	# https://docs.microsoft.com/en-us/cpp/preprocessor/preprocessor-experimental-overview
	if(MSVC AND NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")  # Not allowed on Clang-cl for Visual Studio.
		target_compile_options(Editor PUBLIC /Zc:preprocessor)
	endif()



	# === Build Configs ===

	# Debug config.
	if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")

		message("Build config: Debug")

		# Disable code optimization.
		if(MSVC)
			target_compile_options(Editor PRIVATE /Od)
		else()
			target_compile_options(Editor PRIVATE -Og)
		endif()

		# Create multithreaded debug DLL.
		if(MSVC)
			target_compile_options(Editor PRIVATE /MDd)
		endif()

		# Add Debug information.
		if(MSVC)
			target_compile_options(Editor PRIVATE /Zi)

			if(NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") # Not allowed on Clang-cl for Visual Studio.)
				target_compile_options(Editor PRIVATE /Zf)
			endif()
		else()
			target_compile_options(Editor PRIVATE -g)
		endif()

	# Release config.
	elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	
		message("Build config: Release")

		# Disable RTTI: not for the moment
		#if(MSVC)
		#	target_compile_options(Editor PRIVATE /GR-)
		#else()
		#	target_compile_options(Editor PRIVATE -fno-rtti)
		#endif()

		# Maximum code optimization: O2. O3 may be unstable and build undefined behavior.
		if(MSVC)
			target_compile_options(Editor PRIVATE /O2)
		else()
			target_compile_options(Editor PRIVATE -O2)
		endif()

		# Create multithreaded DLL.
		if(MSVC)
			target_compile_options(Editor PRIVATE /MD)
		endif()

	else()

		message(FATAL_ERROR "Build type: ${CMAKE_BUILD_TYPE} not supported!")

	endif()

endif()

