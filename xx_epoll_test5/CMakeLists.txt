cmake_minimum_required(VERSION 3.10)
project(xx_epoll_test5)
# db

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")

include_directories(
	${PROJECT_SOURCE_DIR}
	../src
	../3rd
	../codegen_test2/cpp
)

add_executable(xx_epoll_test5
		dbdialer.cpp
		dbpeer.cpp
		glistener.cpp
		gpeer.cpp
		ldialer.cpp
		lpeer.cpp
		main.cpp
		peer.cpp
        timer.cpp
		server.cpp
		vpeer.cpp

		../codegen_test2/cpp/pkg_generic.cpp
		../codegen_test2/cpp/pkg_lobby_client.cpp
		../codegen_test2/cpp/pkg_db_service.cpp
		../codegen_test2/cpp/pkg_game_client.cpp
		../codegen_test2/cpp/pkg_game_lobby.cpp

		pkg_game_client_Game1Scene.cpp
		)

target_link_libraries(xx_epoll_test5
		pthread
		readline
		)