project(ServerOmp LANGUAGES CXX)

# Deploy paths are configurable via -D options. When unset we skip the post-build
# deploy step so CI / packaging jobs don't need a real components dir.
if(NOT DEFINED DLL_DEPLOY_DIR)
    set(DLL_DEPLOY_DIR "" CACHE PATH "Optional post-build deploy target for Cef.dll")
endif()
if(NOT DEFINED INC_DEPLOY_DIR)
    set(INC_DEPLOY_DIR "" CACHE PATH "Optional post-build deploy target for cef.inc")
endif()

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
        VERSION "1.4.2.0"
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

if(INC_DEPLOY_DIR)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/src/server/cef.inc"
            "${INC_DEPLOY_DIR}/cef.inc"
        COMMENT "Deploying cef.inc to ${INC_DEPLOY_DIR}"
    )
endif()

if(DLL_DEPLOY_DIR)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PROJECT_NAME}>
            "${DLL_DEPLOY_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMENT "Deploying Cef.dll to ${DLL_DEPLOY_DIR}"
    )
endif()