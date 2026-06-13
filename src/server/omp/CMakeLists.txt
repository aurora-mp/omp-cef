project(ServerOmp LANGUAGES CXX)

# You can change this path for development
#set(DLL_DEPLOY_DIR "${CMAKE_SOURCE_DIR}/tests/omp/components")
#set(INC_DEPLOY_DIR "${CMAKE_SOURCE_DIR}/tests/omp/qawno/include")

set(DLL_DEPLOY_DIR "E:/multiplayer/aurora-mp/rpg/env/components")
set(INC_DEPLOY_DIR "${CMAKE_SOURCE_DIR}/tests/omp/qawno/include")

file(GLOB_RECURSE SERVER_OMP_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_library(${PROJECT_NAME} 
    MODULE 
        ${SERVER_OMP_FILES}
        $<TARGET_OBJECTS:ServerCommon>
)

add_win_version_resource(
    ${PROJECT_NAME}
        VERSION "1.2.0.0"
        COMPANY "aurora-mp"
        PRODUCT_NAME "CEF Server Component"
        FILE_DESC "CEF Component for open.mp"
        INTERNAL_NAME "cef"
        COPYRIGHT "Copyright (c) 2026 Aurora Team"
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/deps/omp-sdk/include
        ${CMAKE_SOURCE_DIR}/deps/pawn/source
        ${CMAKE_SOURCE_DIR}/deps/pawn/source/linux
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OMP-SDK
        Shared
        ServerCommon
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        HAVE_STDINT_H=1
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "Cef"
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