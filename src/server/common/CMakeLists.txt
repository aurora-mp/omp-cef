project(ServerCommon LANGUAGES CXX)

file(GLOB_RECURSE SERVER_COMMON_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

add_library(${PROJECT_NAME} 
    STATIC 
        ${SERVER_COMMON_FILES}
)

if(UNIX AND NOT APPLE)
    set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}/deps
        ${CMAKE_SOURCE_DIR}/deps/pawn-natives
        ${CMAKE_SOURCE_DIR}/deps/samp-plugin-sdk
        ${CMAKE_SOURCE_DIR}/deps/asio/asio/include
        ${CMAKE_SOURCE_DIR}/deps/kcp
)

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Shared
        Threads::Threads
        $<$<PLATFORM_ID:Windows>:ws2_32>
        kcp
        miniz::miniz
        nlohmann_json::nlohmann_json
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        ASIO_STANDALONE
        HAVE_STDINT_H=1
)