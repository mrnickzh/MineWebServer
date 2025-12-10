cmake_minimum_required(VERSION 3.31)
project(MineWebServer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE LIBS "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.hpp")

add_library(MineWebServer STATIC ${SOURCES} ${LIBS})

#target_link_libraries(MineWebServer "${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/zlib.lib")

target_include_directories(MineWebServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)