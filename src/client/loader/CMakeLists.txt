cmake_minimum_required(VERSION 3.21)
project(Loader LANGUAGES CXX)

# You can change this path for development if you want to avoid manually copying and pasting the file (cef.asi)
set(ASI_DEPLOY_DIR "D:/Program Files (x86)/Rockstar Games/GTA San Andreas - Samp Addons")

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
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PROJECT_NAME}>
            "${ASI_DEPLOY_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMENT "Deploying cef.asi to ${ASI_DEPLOY_DIR}"
    )
endif()