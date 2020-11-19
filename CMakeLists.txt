# CMakeList.txt : fichier projet CMake de niveau supérieur, effectuez une configuration globale
# et incluez les sous-projets ici.
#
cmake_minimum_required (VERSION 3.8)

project (DemoEngine VERSION 0.1)

# .lib / .a
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/${CMAKE_BUILD_TYPE}_${CMAKE_HOST_SYSTEM_PROCESSOR}/Libs")

# Module .dll / .so (add_library() with MODULE specifier).
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/${CMAKE_BUILD_TYPE}_${CMAKE_HOST_SYSTEM_PROCESSOR}/Modules")

# .exe
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/${CMAKE_BUILD_TYPE}_${CMAKE_HOST_SYSTEM_PROCESSOR}")

#get_filename_component(INSTALL_ROOT_DIR install ABSOLUTE)
#set(INSTALL_CONFIG_DIR	${INSTALL_ROOT_DIR}/${CMAKE_BUILD_TYPE})
#set(INSTALL_INCLUDE_DIR ${INSTALL_CONFIG_DIR}/include)
#set(INSTALL_BIN_DIR		${INSTALL_CONFIG_DIR}/bin)
#set(INSTALL_LIB_DIR		${INSTALL_CONFIG_DIR}/lib)

#set(CMAKE_INSTALL_PREFIX ${INSTALL_CONFIG_DIR})

#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED True)



#if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
#	set(CMAKE_CXX_FLAGS "-Xclang -std=c++17 /W3 /GX /MDd /Zi /Ob0 /Od /RTC1")
#else()
#	set(CMAKE_CXX_FLAGS "/std:c++17 /EHsc /W4")
#endif()

IF(UNIX AND NOT APPLE)
	set(LINUX TRUE)
ENDIF()


# Vulkan
find_package(Vulkan)

IF(WIN32)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/ThirdParty/vulkan/lib)
		set(Vulkan_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/ThirdParty/vulkan/include)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
ELSEIF(LINUX)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/ThirdParty/vulkan/lib" REQUIRED)
		set(Vulkan_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/ThirdParty/vulkan/include)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
ENDIF()


IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
	message(STATUS ${Vulkan_INCLUDE_DIR})
ENDIF()

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory("ThirdParty/glfw")

# GLM
add_subdirectory("ThirdParty/glm")

# IMGUI
file(GLOB SOURCES "ThirdParty/imgui/imgui.cpp"
	"ThirdParty/imgui/imgui_demo.cpp"
	"ThirdParty/imgui/imgui_draw.cpp"
	"ThirdParty/imgui/imgui_widgets.cpp"
	"ThirdParty/imgui/backends/imgui_impl_glfw.cpp"
	"ThirdParty/imgui/backends/imgui_impl_vulkan.cpp")

add_library(imgui INTERFACE)

target_sources(imgui INTERFACE ${SOURCES})
target_include_directories(imgui INTERFACE "ThirdParty/imgui")

# TINYOBJLOADER
add_subdirectory("ThirdParty/tinyobjloader")

# ENGINE
add_subdirectory ("Engine")
