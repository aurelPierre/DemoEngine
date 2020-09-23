# CMakeList.txt : projet CMake pour EzUtils, incluez la source et définissez
# la logique spécifique au projet ici.
#
cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE SOURCES src/*)

# EzUtils
add_executable(EzUtils ${SOURCES})

target_sources(EzUtils PUBLIC ${SOURCES})

target_link_libraries(EzUtils PUBLIC glm)
target_link_libraries(EzUtils PUBLIC imgui)
target_link_libraries(EzUtils PUBLIC "${PROJECT_SOURCE_DIR}/Lib/glfw/lib/glfw3.lib"
										"${PROJECT_SOURCE_DIR}/Lib/vulkan/lib/vulkan-1.lib")
	
target_include_directories(EzUtils PUBLIC	"${CMAKE_CURRENT_SOURCE_DIR}/include"
											"${CMAKE_CURRENT_SOURCE_DIR}/include/VkRenderer"
											"${PROJECT_BINARY_DIR}"
											"${PROJECT_SOURCE_DIR}/Lib/glfw/include"
											"${PROJECT_SOURCE_DIR}/Lib/vulkan/include")

set_target_properties(EzUtils PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Lib)

# INSTALL
install(DIRECTORY include/	DESTINATION ${INSTALL_INCLUDE_DIR})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/Lib/glfw/include/	DESTINATION ${INSTALL_INCLUDE_DIR}/external/glfw)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/Lib/vulkan/include/	DESTINATION ${INSTALL_INCLUDE_DIR}/external/vulkan)

install(TARGETS EzUtils EXPORT EzUtils			ARCHIVE DESTINATION ${INSTALL_BIN_DIR}
												RUNTIME DESTINATION ${INSTALL_BIN_DIR})

if (CMAKE_BUILD_TYPE MATCHES "Debug")
	set(SYSTEM_FLAG "")
else()
	set(SYSTEM_FLAG "/SUBSYSTEM:WINDOWS /entry:mainCRTStartup")
endif()