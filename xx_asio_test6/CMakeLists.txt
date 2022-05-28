cmake_minimum_required(VERSION 3.10)
project(xx_asio_test6)

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
	add_definitions("-D_WIN32_WINNT=0x0602")	# for asio
endif ()

add_definitions("-DONELUA_MAKE_LIB")

list(APPEND dirs
		../src
		../3rd
		../asio_include
		../lua-5.4.x/src
		)
list(APPEND srcs
		main.cpp
		../lua-5.4.x/src/onelua.cpp
		)


link_directories(
		${libdirs}
)
include_directories(
		${dirs}
)
add_executable(xx_asio_test6
		${srcs}
		)
target_link_libraries(xx_asio_test6
		${libs}
		)