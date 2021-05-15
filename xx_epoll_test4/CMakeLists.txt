cmake_minimum_required(VERSION 3.10)
project(xx_epoll_test4)
# db

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")

include_directories(
	../src
	../3rd
	../codegen_test2/cpp
)

add_executable(xx_epoll_test4
		main.cpp
		server.cpp
		peer.cpp
		listener.cpp
		db.cpp
		../codegen_test2/cpp/pkg_generic.cpp
		../codegen_test2/cpp/pkg_db_service.cpp
		)

target_link_libraries(xx_epoll_test4
		pthread
		readline
		sqlite3
		)