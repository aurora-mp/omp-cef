cmake_minimum_required(VERSION 3.21)

project(OmpCef LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(BUILD_CLIENT "Build the client" OFF)
option(BUILD_SERVER_OMP "Build the open.mp component" OFF)
option(BUILD_SERVER_SAMP "Build the SA-MP plugin" OFF)

if (WIN32)
	add_compile_definitions(_WIN32_WINNT=0x0A00 NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
	add_compile_options(/MP)
endif()

if (MSVC)
	# /MT (Release) and /MTd (Debug) - consistent with x86-windows-static triplet.
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

list(APPEND CMAKE_MODULE_PATH
	"${CMAKE_SOURCE_DIR}/cmake"
	"${CMAKE_SOURCE_DIR}/deps/samp-cmake"
)

include(AddWinVersion)

# Arch detection (client & samp are x86 only)
set(IS_32BIT OFF)
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
  	set(IS_32BIT ON)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/deps/kcp ${CMAKE_BINARY_DIR}/_deps_kcp)
add_subdirectory(src/shared)

if(BUILD_SERVER_OMP OR BUILD_SERVER_SAMP)
    add_subdirectory(src/server)
endif()

# Client (x86 only)
if (IS_32BIT)
	if (BUILD_CLIENT)
		add_subdirectory(src/client)
	endif()
endif()