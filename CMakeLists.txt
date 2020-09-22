# CMakeList.txt : fichier projet CMake de niveau supérieur, effectuez une configuration globale
# et incluez les sous-projets ici.
#
cmake_minimum_required (VERSION 3.8)

get_filename_component(INSTALL_ROOT_DIR install ABSOLUTE)
set(INSTALL_CONFIG_DIR	${INSTALL_ROOT_DIR}/${CMAKE_BUILD_TYPE})
set(INSTALL_INCLUDE_DIR ${INSTALL_CONFIG_DIR}/include)
set(INSTALL_BIN_DIR		${INSTALL_CONFIG_DIR}/bin)
set(INSTALL_LIB_DIR		${INSTALL_CONFIG_DIR}/lib)

set(CMAKE_INSTALL_PREFIX ${INSTALL_CONFIG_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project (EzEngine VERSION 0.1)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "-Xclang -std=c++17 /W3 /GX /MDd /Zi /Ob0 /Od /RTC1")
else()
	set(CMAKE_CXX_FLAGS "/std:c++17 /EHsc /W4")
endif()

# Incluez les sous-projets.
add_subdirectory ("Lib")
add_subdirectory ("Utils")
add_subdirectory ("EzUtils")
