cmake_minimum_required(VERSION 3.10)
project(xx_asio_testC)

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

list(APPEND dirs
		../../boost
		../src
		../3rd
		../xx_asio_testA
		)
list(APPEND srcs
		main.cpp
		../xx_asio_testA/pkg.cpp
		)


link_directories(
		${libdirs}
)
include_directories(
		${dirs}
)
add_executable(xx_asio_testC
		${srcs}
		)
target_link_libraries(xx_asio_testC
		${libs}
		)