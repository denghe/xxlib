cmake_minimum_required(VERSION 3.10)
project(xx_lua_test2)

set(CMAKE_CXX_STANDARD 20)

add_definitions("-DMAKE_LIB")

set(srcs)
set(dirs)
set(libdirs)

list(APPEND dirs
        ../src
        ../3rd
        ../lua-5.4.x/src
        )
list(APPEND srcs
        main.cpp
        ../lua-5.4.x/src/onelua.cpp
        )

if (UNIX)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif ()

link_directories(
        ${libdirs}
)
include_directories(
        ${dirs}
)
add_executable(xx_lua_test2
        ${srcs}
        )

#if(UNIX)
#    find_package(mimalloc 1.7 REQUIRED)
#    target_link_libraries(xx_lua_test2 PUBLIC mimalloc-static)
#endif()
