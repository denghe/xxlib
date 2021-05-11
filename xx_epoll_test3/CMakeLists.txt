cmake_minimum_required(VERSION 3.10)
project(xx_epoll_test3)
# lobby

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")

include_directories(
	../src
	../3rd
	../codegen_test2/cpp
)

add_executable(xx_epoll_test3
		main.cpp
		server.cpp
		peer.cpp
		listener.cpp
		apeer.cpp
		gpeer.cpp
		vpeer.cpp
		pingtimer.cpp
		db.cpp
		../codegen_test2/cpp/pkg_generic.cpp
		../codegen_test2/cpp/pkg_lobby.cpp
		)

target_link_libraries(xx_epoll_test3
		pthread
		readline
		sqlite3
		)