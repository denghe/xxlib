cmake_minimum_required(VERSION 3.10)
project(xx_mysql_test1)

set(CMAKE_CXX_STANDARD 20)

if(UNIX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
else()
	include_directories(
		"C:/Program Files/MariaDB/MariaDB Connector C 64-bit/include"
	)
endif()

include_directories(
	../src
)

add_executable(xx_mysql_test1
        main.cpp
        )

