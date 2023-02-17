cmake_minimum_required(VERSION 3.10)
project(xx_asio_testG)

set(CMAKE_CXX_STANDARD 20)

set(srcs)
set(dirs)
set(libdirs)
set(libs)

# global
if (UNIX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
	list(APPEND libs
			pthread
			)
else()
	add_definitions("-D_WIN32_WINNT=0x0602")	# for asio 0602 win8
endif ()

add_definitions("-DASIO_CONCURRENCY_HINT_1=ASIO_CONCURRENCY_HINT_UNSAFE")

list(APPEND dirs
		../../boost
		../src
		../3rd
		)
list(APPEND srcs
		main.cpp
		../3rd/ikcp.c
		)

link_directories(
		${libdirs}
)
include_directories(
		${dirs}
)
add_executable(xx_asio_testG
		${srcs}
		)
target_link_libraries(xx_asio_testG
		${libs}
		)