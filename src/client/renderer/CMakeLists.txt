cmake_minimum_required(VERSION 3.21)
project(Renderer LANGUAGES CXX)

set(CEF_GAME_DIR "" CACHE PATH "Optional GTA San Andreas directory for local post-build deployment.")
set(CEF_RENDERER_DEPLOY_DIR "" CACHE PATH "Optional post-build deploy target for renderer.exe.")

if("${CEF_GAME_DIR}" STREQUAL "" AND DEFINED ENV{CEF_GAME_DIR} AND NOT "$ENV{CEF_GAME_DIR}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{CEF_GAME_DIR}" CEF_GAME_DIR_FROM_ENV)
    set(CEF_GAME_DIR "${CEF_GAME_DIR_FROM_ENV}" CACHE PATH "Optional GTA San Andreas directory for local post-build deployment." FORCE)
endif()

if("${CEF_RENDERER_DEPLOY_DIR}" STREQUAL "" AND NOT "${CEF_GAME_DIR}" STREQUAL "")
    set(CEF_RENDERER_DEPLOY_DIR "${CEF_GAME_DIR}/nsy")
endif()

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
    set(POST_BUILD_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/deploy"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${CMAKE_SOURCE_DIR}/deploy/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    )
    if(CEF_RENDERER_DEPLOY_DIR)
        list(APPEND POST_BUILD_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CEF_RENDERER_DEPLOY_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${CEF_RENDERER_DEPLOY_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        )
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        ${POST_BUILD_COMMANDS}
        COMMENT "Deploying renderer.exe"
    )
endif()
