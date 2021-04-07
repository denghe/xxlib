cmake_minimum_required(VERSION 3.10)
project(xx_obj_test1)

set(CMAKE_CXX_STANDARD 20)
if(UNIX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

include_directories(
	../src
	../codegen_test1/cpp
)

add_executable(xx_obj_test1
        main.cpp
		../codegen_test1/cpp/shared.cpp
		../codegen_test1/cpp/p1.cpp
		../codegen_test1/cpp/foo.cpp
        )
