cmake_minimum_required(VERSION 3.10)
project(xx_lua_test1)

set(CMAKE_CXX_STANDARD 20)

add_definitions("-DMAKE_LIB")

set(srcs)
set(dirs)
set(libdirs)

# global
list(APPEND dirs
        ../src
        ../3rd
        "../libuv/include"
        ../lua-5.4.x/src
        )
list(APPEND libdirs
        ../libuv/lib/win64
        )
list(APPEND srcs
        main.cpp
        ../3rd/ikcp.c
        ../lua-5.4.x/src/onelua.cpp
        )

##lua socket
#set(luasocket_srcs)
#list(APPEND dirs
#        ../luasocket-2.0.2/src
#        )
#list(APPEND luasocket_srcs
#        ../luasocket-2.0.2/src/buffer.c
#        ../luasocket-2.0.2/src/io.c
#        ../luasocket-2.0.2/src/timeout.c
#        ../luasocket-2.0.2/src/select.c
#        ../luasocket-2.0.2/src/udp.c
#        ../luasocket-2.0.2/src/mime.c
#        ../luasocket-2.0.2/src/tcp.c
#        ../luasocket-2.0.2/src/auxiliar.c
#        ../luasocket-2.0.2/src/inet.c
#        ../luasocket-2.0.2/src/luasocket.c
#        ../luasocket-2.0.2/src/except.c
#        ../luasocket-2.0.2/src/options.c
#        )
#if (UNIX)
#    list(APPEND luasocket_srcs
#            ../luasocket-2.0.2/src/unix.c
#            ../luasocket-2.0.2/src/usocket.c
#            ../luasocket-2.0.2/src/serial.c
#            )
#else ()
#    list(APPEND luasocket_srcs
#            ../luasocket-2.0.2/src/wsocket.c
#            )
#endif ()
#set_source_files_properties(${luasocket_srcs} PROPERTIES LANGUAGE CXX)
#list(APPEND srcs
#		${luasocket_srcs}
#		)

# global
if (UNIX)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif ()

link_directories(
        ${libdirs}
)
include_directories(
        ${dirs}
)
add_executable(xx_lua_test1
        ${srcs}
        )
