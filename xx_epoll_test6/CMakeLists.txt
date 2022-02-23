cmake_minimum_required(VERSION 3.10)
project(xx_epoll_test6)
# simple echo server

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto -static")

include_directories(
	../src
	../3rd
)

add_executable(xx_epoll_test6
		main.cpp
		)

target_link_libraries(xx_epoll_test6
		pthread
		#readline
		)