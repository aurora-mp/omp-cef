cmake_minimum_required(VERSION 3.21)
project(Loader LANGUAGES CXX)

set(CEF_GAME_DIR "" CACHE PATH "Optional GTA San Andreas directory for local post-build deployment.")
set(CEF_ASI_DEPLOY_DIR "" CACHE PATH "Optional post-build deploy target for cef.asi.")

if("${CEF_GAME_DIR}" STREQUAL "" AND DEFINED ENV{CEF_GAME_DIR} AND NOT "$ENV{CEF_GAME_DIR}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{CEF_GAME_DIR}" CEF_GAME_DIR_FROM_ENV)
    set(CEF_GAME_DIR "${CEF_GAME_DIR_FROM_ENV}" CACHE PATH "Optional GTA San Andreas directory for local post-build deployment." FORCE)
endif()

if("${CEF_ASI_DEPLOY_DIR}" STREQUAL "" AND NOT "${CEF_GAME_DIR}" STREQUAL "")
    set(CEF_ASI_DEPLOY_DIR "${CEF_GAME_DIR}")
endif()

add_library(${PROJECT_NAME} SHARED main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "nsy"
    SUFFIX ".asi"
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        WIN32_LEAN_AND_MEAN
        VC_EXTRALEAN
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        shlwapi
)

if(NOT DEFINED ENV{CI})
    set(POST_BUILD_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/deploy"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${CMAKE_SOURCE_DIR}/deploy/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    )
    if(CEF_ASI_DEPLOY_DIR)
        list(APPEND POST_BUILD_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CEF_ASI_DEPLOY_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${CEF_ASI_DEPLOY_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        )
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        ${POST_BUILD_COMMANDS}
        COMMENT "Deploying nsy.asi"
    )
endif()
