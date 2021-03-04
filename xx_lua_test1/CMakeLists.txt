cmake_minimum_required(VERSION 3.10)
project(xx_lua_test1)

set(CMAKE_CXX_STANDARD 20)

if(UNIX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")

#	target_link_libraries(xx_lua_test1
#        dl
#        pthread
#        luajit-5.1
#    )
#add_definitions("-DUSING_LUAJIT")

## lua 5.4
#include_directories(
#        ../xxlib
#        ../lua-5.4.x/src
#)
#set(LUA_SRC ${PROJECT_SOURCE_DIR}/../lua-5.4.x/src/onelua.c)
#set_source_files_properties(${LUA_SRC} PROPERTIES LANGUAGE CXX)
#add_definitions("-DMAKE_LIB -DLUA_USE_LINUX")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto -fuse-ld=gold")
#add_executable(gateway_client_lua
#        main.cpp
#        FileExts_class_lite.cpp
#        Objs_class_lite.cpp
#        ${LUA_SRC}
#        )
#target_link_libraries(gateway_client_lua
#        dl
#        pthread
#        readline
#        )
endif()


include_directories(
	../src
    ../lua-5.4.x/src
)

add_executable(xx_lua_test1
    main.cpp
    ../lua-5.4.x/src/onelua.cpp
)
