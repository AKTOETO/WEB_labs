cmake_minimum_required(VERSION 3.22)
project(multi)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME})

add_executable(${PROJECT_NAME} ./multi.cpp ./gen.h)

project(single)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME})

add_executable(${PROJECT_NAME} ./single.cpp ./gen.h)
