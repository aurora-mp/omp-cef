project(ServerSamp LANGUAGES CXX)

# You can change this path for development
set(DLL_DEPLOY_DIR "${CMAKE_SOURCE_DIR}/tests/samp/plugins")
set(INC_DEPLOY_DIR "${CMAKE_SOURCE_DIR}/tests/samp/pawno/include")

set(SAMP_SDK_ROOT "${CMAKE_SOURCE_DIR}/deps/samp-plugin-sdk")
find_package(SAMPSDK REQUIRED)
include_directories(${SAMPSDK_DIR})

file(GLOB_RECURSE SERVER_SAMP_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_library(${PROJECT_NAME} 
    MODULE 
        ${SERVER_SAMP_FILES}
        $<TARGET_OBJECTS:ServerCommon>
		${SAMPSDK_DIR}/amxplugin.cpp
        $<$<PLATFORM_ID:Windows>:plugin.def>
)

add_win_version_resource(
    ${PROJECT_NAME}
        VERSION "1.2.0.0"
        COMPANY "aurora-mp"
        PRODUCT_NAME "SA-MP Server Plugin"
        FILE_DESC "CEF Plugin for SA-MP"
        INTERNAL_NAME "cef"
        COPYRIGHT "Copyright (C) 2026 Aurora Team"
)

target_include_directories(${PROJECT_NAME} 
    PRIVATE
        ${CMAKE_SOURCE_DIR}/deps/samp-plugin-sdk
        ${CMAKE_SOURCE_DIR}/deps/samp-plugin-sdk/amx
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        sampgdk
        Shared
        ServerCommon
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE LINUX)
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        HAVE_STDINT_H=1
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "cef"
    PREFIX ""
)

if(NOT DEFINED ENV{CI})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/src/server/cef.inc"
            "${INC_DEPLOY_DIR}/cef.inc"
        COMMENT "Deploying cef.inc to ${INC_DEPLOY_DIR}"
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PROJECT_NAME}>
            "${DLL_DEPLOY_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMENT "Deploying Cef.dll to ${DLL_DEPLOY_DIR}"
    )
endif()