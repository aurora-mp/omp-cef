cmake_minimum_required(VERSION 3.21)
project(Renderer LANGUAGES CXX)

# You can change this path for development if you want to avoid manually copying and pasting the file (renderer.exe)
set(FILE_DEPLOY_DIR "D:/Program Files (x86)/Rockstar Games/GTA San Andreas - Samp Addons/cef")

set(RENDERER_SRC main.cpp)
add_executable(${PROJECT_NAME} WIN32 ${RENDERER_SRC})

add_win_version_resource(
    ${PROJECT_NAME}
        VERSION "1.2.0.0"
        COMPANY "aurora-mp"
        PRODUCT_NAME "OMP CEF Renderer"
        FILE_DESC "CEF renderer subprocess"
        INTERNAL_NAME "renderer"
        COPYRIGHT "Copyright (c) 2025 Aurora Team"
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CEF_ROOT})

target_link_libraries(${PROJECT_NAME} PRIVATE
    libcef_lib
    libcef_dll_wrapper
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    OUTPUT_NAME "renderer"
)

if(NOT DEFINED ENV{CI})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${FILE_DEPLOY_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMENT "Copying renderer.exe to ${FILE_DEPLOY_DIR}"
    )
endif()