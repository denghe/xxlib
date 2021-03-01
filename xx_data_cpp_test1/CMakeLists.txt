cmake_minimum_required(VERSION 3.10)
project(xx_data_cpp_test1)

set(CMAKE_CXX_STANDARD 20)

include_directories(
	../src
)

add_executable(xx_data_cpp_test1
        main.cpp
        )
