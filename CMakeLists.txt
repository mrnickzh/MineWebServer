cmake_minimum_required(VERSION 3.31)
project(MineWebServer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE LIBS "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.hpp")

add_library(MineWebServer STATIC ${SOURCES} ${LIBS})

if (BUILD_TYPE_DEDICATED)
    FetchContent_Declare(
            zlib
            GIT_REPOSITORY https://github.com/madler/zlib.git
            GIT_TAG        master
    )

    FetchContent_MakeAvailable(zlib)

    target_link_libraries(MineWebServer PUBLIC zlibstatic)
endif ()

target_include_directories(MineWebServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)