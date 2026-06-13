cmake_minimum_required(VERSION 3.21)
project(ClientProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CEF
set(CEF_VERSION "143.0.14+gdd46a37+chromium-143.0.7499.193" CACHE STRING "")
include(${CMAKE_SOURCE_DIR}/cmake/cef.cmake)

if(DEFINED PREBUILT_LIBCEF_DLL_WRAPPER)
    add_library(libcef_dll_wrapper STATIC IMPORTED GLOBAL)
    set_target_properties(libcef_dll_wrapper PROPERTIES
        IMPORTED_LOCATION "${PREBUILT_LIBCEF_DLL_WRAPPER}"
        INTERFACE_INCLUDE_DIRECTORIES "${CEF_ROOT}/include"
    )
else()
    add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)
endif()

set_target_properties(libcef_dll_wrapper PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

if (NOT TARGET libcef_dll_wrapper OR NOT TARGET libcef_lib)
    message(FATAL_ERROR "CEF targets not found: 'libcef_dll_wrapper' or/and 'libcef_lib'.")
endif()

# Deps
if (NOT TARGET Shared)
    # add_subdirectory(${CMAKE_SOURCE_DIR}/src/shared ${CMAKE_BINARY_DIR}/_shared)
endif()

if (NOT TARGET sampapi)
    add_subdirectory(${CMAKE_SOURCE_DIR}/deps/samp-api ${CMAKE_BINARY_DIR}/_deps_sampapi)
endif()

if (NOT TARGET tiny-aes)
    add_subdirectory(${CMAKE_SOURCE_DIR}/deps/tiny-aes ${CMAKE_BINARY_DIR}/_deps_tinyaes)
endif()

# Sub-projects
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/core) # client.dll
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/loader) # cef.asi
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/renderer) # renderer.exe