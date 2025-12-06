cmake_minimum_required(VERSION 3.31)
project(MineWebServer)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")

add_library(MineWebServer STATIC ${SOURCES})

target_include_directories(MineWebServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)