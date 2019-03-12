#
# Tyler Filla
# CS 4500 HW4
#

cmake_minimum_required(VERSION 3.8)
project(CS4500HW4)

add_subdirectory(third_party/glfw3)

add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

add_library(nanovg STATIC third_party/nanovg/src/nanovg.c)
target_include_directories(nanovg PUBLIC third_party/nanovg/include)

add_executable(main main.c)
target_include_directories(main PRIVATE third_party/glfw3/include)
target_link_libraries(main PRIVATE glad glfw nanovg)
