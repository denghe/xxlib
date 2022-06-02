cmake_minimum_required(VERSION 3.10)
project(xx_sqlite_test1)

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

add_definitions("-DSQLITE_OMIT_PROGRESS_CALLBACK")
add_definitions("-DSQLITE_OMIT_PROGRESS_CALLBACK")
add_definitions("-DSQLITE_DEFAULT_MEMSTATUS=0")
add_definitions("-DSQLITE_USE_ALLOCA")
add_definitions("-DSQLITE_LIKE_DOESNT_MATCH_BLOBS")
add_definitions("-DSQLITE_OMIT_DEPRECATED")
add_definitions("-DSQLITE_OMIT_AUTOINIT")
add_definitions("-DSQLITE_OMIT_LOAD_EXTENSION")
add_definitions("-DSQLITE_DQS=0")
add_definitions("-DSQLITE_MAX_EXPR_DEPTH=0")
add_definitions("-DSQLITE_THREADSAFE=0")

list(APPEND dirs
		../src
		../3rd
		../sqlite3
		../../boost
		)
list(APPEND srcs
		main.cpp
		../sqlite3/sqlite3.c
		)


link_directories(
		${libdirs}
)
include_directories(
		${dirs}
)
add_executable(xx_sqlite_test1
		${srcs}
		)
target_link_libraries(xx_sqlite_test1
		${libs}
		)