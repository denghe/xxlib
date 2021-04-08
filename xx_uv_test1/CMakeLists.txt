cmake_minimum_required(VERSION 3.10)
project(xx_uv_test1)

set(CMAKE_CXX_STANDARD 20)
if(UNIX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

link_directories(
		../libuv/lib/win64
)

include_directories(
	../src
	../3rd
	"../libuv/include"
)

add_executable(xx_uv_test1
        main.cpp
        )

#target_link_libraries(xx_uv_test1
		#pthread
		#readline
		#
		#)