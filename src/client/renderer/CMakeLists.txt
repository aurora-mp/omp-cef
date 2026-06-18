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

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "renderer"
)

if(NOT DEFINED ENV{CI} AND CEF_RENDERER_DEPLOY_DIR)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CEF_RENDERER_DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CEF_RENDERER_DEPLOY_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_SOURCE_DIR}/deploy"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CMAKE_SOURCE_DIR}/deploy/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMENT "Deploying renderer.exe to ${CEF_RENDERER_DEPLOY_DIR} and /deploy"
    )
endif()
