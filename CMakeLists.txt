cmake_minimum_required(VERSION 3.23)
project(lw10)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ищем нужные библиотеки
find_package(Boost REQUIRED)

# объявляем исходные файлы для библиотеки
set(CLIENT_SRC "src/client.cpp" "src/msg.hpp")
set(SERVER_SRC "src/server.cpp" "src/msg.hpp")

add_executable(CLIENT ${CLIENT_SRC})
add_executable(SERVER ${SERVER_SRC})

target_link_libraries(CLIENT PUBLIC boost::boost)
target_link_libraries(SERVER PUBLIC boost::boost)