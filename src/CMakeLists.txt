cmake_minimum_required(VERSION 3.8)

project(xxlib)

set(XXLIB_SRCS)
file(GLOB_RECURSE XXLIB_SRCS
	*.h
    empty.cpp
)

add_library(${PROJECT_NAME} STATIC
    ${XXLIB_SRCS}
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
