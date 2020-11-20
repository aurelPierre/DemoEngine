# CMakeList.txt : projet CMake pour EzUtils, incluez la source et définissez
# la logique spécifique au projet ici.
#
cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE SOURCES src/*)

# Engine
add_library(Engine ${SOURCES})

if(CODE_COVERAGE)
	set_target_properties(Engine PROPERTIES LINK_FLAGS "-lgcov --coverage")
	target_code_coverage(Engine EXCLUDE test/*)
endif()

target_sources(Engine PUBLIC ${SOURCES})

# Third Party
target_link_libraries(Engine PUBLIC glm)
target_link_libraries(Engine PUBLIC ${Vulkan_LIBRARY})
target_link_libraries(Engine PUBLIC glfw)
target_link_libraries(Engine PUBLIC imgui)
target_link_libraries(Engine PUBLIC tinyobjloader)
target_link_libraries(Engine PUBLIC stb)

target_include_directories(Engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(Engine PUBLIC "${Vulkan_INCLUDE_DIR}")


# INSTALL
#install(DIRECTORY include/	DESTINATION ${INSTALL_INCLUDE_DIR})
#install(DIRECTORY ${PROJECT_SOURCE_DIR}/Lib/glfw/include/	DESTINATION ${INSTALL_INCLUDE_DIR}/external/glfw)
#install(DIRECTORY ${PROJECT_SOURCE_DIR}/Lib/vulkan/include/	DESTINATION ${INSTALL_INCLUDE_DIR}/external/vulkan)

#install(TARGETS Engine EXPORT Engine			ARCHIVE DESTINATION ${INSTALL_BIN_DIR}
#												RUNTIME DESTINATION ${INSTALL_BIN_DIR})

# === Dependencies ===

# Enable Multithreading (-pthread).

# === Compile features ===

# Clean default compilation flags.
if(MSVC)
	#set(CMAKE_C_FLAGS "/DWIN32 /D_WINDOWS")
	#set(CMAKE_C_FLAGS_RELEASE "/DNDEBUG")
	#set(CMAKE_C_FLAGS_DEBUG "")

	#set(CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS")
	#set(CMAKE_CXX_FLAGS_RELEASE "/DNDEBUG")
	#set(CMAKE_CXX_FLAGS_DEBUG "")
endif()


# Standard
target_compile_features(Engine PUBLIC c_std_11)
target_compile_features(Engine PUBLIC cxx_std_17)

# Warnings
if(MSVC)
	target_compile_options(Engine PRIVATE /W4)
else()
	target_compile_options(Engine PRIVATE -Wall -Wextra)
endif()

# Multithreaded compilation.
if(MSVC AND NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") # Not allowed on Clang-cl for Visual Studio.
	target_compile_options(Engine PRIVATE /MP)
endif()

# Exceptions handling.
if(MSVC)
	target_compile_options(Engine PRIVATE /EHsc)
else()
	target_compile_options(Engine PRIVATE -fexceptions)
endif()

# Advanced MSVC preprocessor required for Core::Debug
# https://docs.microsoft.com/en-us/cpp/preprocessor/preprocessor-experimental-overview
if(MSVC AND NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")  # Not allowed on Clang-cl for Visual Studio.
	target_compile_options(Engine PUBLIC /Zc:preprocessor)
endif()



# === Build Configs ===

# Debug config.
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")

	message("Build config: Debug")

	# Disable code optimization.
	if(MSVC)
		target_compile_options(Engine PRIVATE /Od)
	else()
		target_compile_options(Engine PRIVATE -Og)
	endif()

	# Create multithreaded debug DLL.
	if(MSVC)
		target_compile_options(Engine PRIVATE /MDd)
	endif()

	# Add Debug information.
	if(MSVC)
		target_compile_options(Engine PRIVATE /Zi)

		if(NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") # Not allowed on Clang-cl for Visual Studio.)
			target_compile_options(Engine PRIVATE /Zf)
		endif()
	else()
		target_compile_options(Engine PRIVATE -g)
	endif()

# Release config.
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	
	message("Build config: Release")

	# Disable RTTI: not for the moment
	#if(MSVC)
	#	target_compile_options(Engine PRIVATE /GR-)
	#else()
	#	target_compile_options(Engine PRIVATE -fno-rtti)
	#endif()

	# Maximum code optimization: O2. O3 may be unstable and build undefined behavior.
	if(MSVC)
		target_compile_options(Engine PRIVATE /O2)
	else()
		target_compile_options(Engine PRIVATE -O2)
	endif()

	# Create multithreaded DLL.
	if(MSVC)
		target_compile_options(Engine PRIVATE /MD)
	endif()

else()

	message(FATAL_ERROR "Build type: ${CMAKE_BUILD_TYPE} not supported!")

endif()

add_subdirectory("test")