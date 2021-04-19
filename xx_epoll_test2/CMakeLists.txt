cmake_minimum_required(VERSION 3.10)
project(xx_epoll_test2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")

include_directories(
	../src
	../3rd
)

add_executable(xx_epoll_test2
		main.cpp
		server.cpp
		peer.cpp
		listener.cpp
		dialer.cpp
		cpeer.cpp
		speer.cpp
		pingtimer.cpp
		tasktimer.cpp
		kpeer.cpp
		../3rd/ikcp.c
		)

target_link_libraries(xx_epoll_test2
		pthread
		readline
		)