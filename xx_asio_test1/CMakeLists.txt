cmake_minimum_required(VERSION 3.10)
project(xx_asio_test1)

set(CMAKE_CXX_STANDARD 20)

set(srcs)
set(dirs)
set(libdirs)
set(libs)

# global
if (UNIX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
else()
	add_definitions("-D_WIN32_WINNT=0x0601")	# for asio
endif ()

list(APPEND dirs
		../src
		../3rd
		../asio_include
		)
list(APPEND srcs
		main.cpp
		../3rd/ikcp.c
		)
list(APPEND libs
		pthread
		)


link_directories(
		${libdirs}
)
include_directories(
		${dirs}
)
add_executable(xx_asio_test1
		${srcs}
		)
target_link_libraries(xx_asio_test1
		${libs}
		)