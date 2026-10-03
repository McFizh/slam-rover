cmake_minimum_required(VERSION 3.25)
project(dashboard)

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(dashboard src/main.cpp src/ui.cpp src/controller.cpp src/network.cpp)
target_link_libraries(dashboard PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)

add_custom_command(TARGET dashboard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:dashboard>/assets
)