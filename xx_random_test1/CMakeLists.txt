cmake_minimum_required(VERSION 3.10)
project(xx_random_test1)

set(CMAKE_CXX_STANDARD 20)
if(UNIX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

include_directories(
	../src
	../3rd
)

add_executable(xx_random_test1
        main.cpp
        )

#find_package(mimalloc 1.7 REQUIRED)
#target_link_libraries(xx_random_test1 PUBLIC mimalloc-static)
