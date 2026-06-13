project(Shared LANGUAGES CXX)

file(GLOB_RECURSE SHARED_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

find_package(unofficial-sodium CONFIG REQUIRED)
find_path(TINYAES_INCLUDE_DIRS aes.hpp
    HINTS
        ${CMAKE_SOURCE_DIR}/deps/tiny-aes/include
        ${CMAKE_SOURCE_DIR}/deps/tiny-aes
)

find_path(PICOSHA2_INCLUDE_DIRS picosha2.h)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${PICOSHA2_INCLUDE_DIRS}
        ${TINYAES_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        unofficial-sodium::sodium
        tiny-aes
)