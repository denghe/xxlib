cmake_minimum_required(VERSION 3.10)
project(xx_math_test1)

set(CMAKE_CXX_STANDARD 20)
if(UNIX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

include_directories(
	../src
)

add_executable(xx_math_test1
        main.cpp
        )
if(UNIX)
	target_link_libraries(xx_math_test1
			pthread
			)
endif()
