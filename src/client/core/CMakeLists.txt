cmake_minimum_required(VERSION 3.21)
project(Client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# You can change this path for development if you want to avoid manually copying and pasting the file (renderer.exe)
set(DLL_DEPLOY_DIR "D:/Program Files (x86)/Rockstar Games/GTA San Andreas - Samp Addons/cef")

# Deps
#find_package(asio CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(miniz CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)
find_package(minhook CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)

if (NOT TARGET libcef_dll_wrapper OR NOT TARGET libcef_lib)
    message(FATAL_ERROR "CEF not initialized (missing libcef_dll_wrapper/libcef_lib).")
endif()

file(GLOB_RECURSE CLIENT_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(${PROJECT_NAME} SHARED ${CLIENT_SRC})

add_win_version_resource(
    ${PROJECT_NAME}
        VERSION "1.2.0.0"
        COMPANY "aurora-mp"
        PRODUCT_NAME "CEF Client Plugin"
        FILE_DESC "CEF Client DLL for SA-MP/open.mp"
        INTERNAL_NAME "client"
        COPYRIGHT "Copyright (c) 2026 Aurora Team"
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CEF_ROOT}
        ${CMAKE_SOURCE_DIR}/deps/asio/asio/include
        ${CMAKE_SOURCE_DIR}/deps/kcp
)

# DirectX
if (DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    target_include_directories(${PROJECT_NAME}
        PRIVATE
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/directxsdk"
    )

    target_link_directories(${PROJECT_NAME}
        PRIVATE
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib"
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        user32
        Version
        ws2_32
        d3d9
        d3dx9

        Shared
        sampapi
        tiny-aes

        libcef_lib
        libcef_dll_wrapper

        #asio::asio
        fmt::fmt
        nlohmann_json::nlohmann_json
        miniz::miniz
        minhook::minhook
        OpenAL::OpenAL
        unofficial-sodium::sodium
        kcp
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        ASIO_STANDALONE
)

# PCH
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp")
    target_precompile_headers(${PROJECT_NAME} PRIVATE pch.hpp)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "client"
)

if(NOT DEFINED ENV{CI})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${DLL_DEPLOY_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMENT "Copying client.dll to ${DLL_DEPLOY_DIR}"
    )
endif()
