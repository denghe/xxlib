cmake_minimum_required(VERSION 3.10)
project(xx_asio_test2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")

include_directories(
	../src
	../3rd
	../codegen_test3/cpp
)

add_executable(xx_asio_test2
		main.cpp
		server.cpp
		listener.cpp
		peer.cpp
		../3rd/ikcp.c
		../codegen_test3/cpp/ss.cpp
		ss_ex.cpp
		)

find_package(mimalloc 1.7 REQUIRED)

target_link_libraries(xx_asio_test2
		pthread
		readline
		mimalloc-static
		)
