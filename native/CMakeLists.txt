cmake_minimum_required(VERSION 3.21)

set(CROSS_LINUX TRUE)

if(CROSS_LINUX)
	MESSAGE(STATUS "Cross Platform Linux build running...")
	# the name of the target operating system
	set(CMAKE_CROSSCOMPILING TRUE)
	set(CMAKE_SYSTEM_NAME Linux)
	set(CMAKE_SYSTEM_PROCESSOR arm)

	# adjust the default behavior of the find commands:
	# search headers and libraries in the target environment
#	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
#	set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	# search programs in the host environment
#	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
endif()

project(Sync LANGUAGES CXX)

set(CXX_STANDARD 14)
add_compile_definitions(LIVE)
add_compile_definitions(WINDOWS)
add_compile_definitions(LITTLE_ENDIAN)
add_compile_definitions(_WIN32_WINNT=0x0601)
add_compile_definitions(BOOST_BIND_NO_PLACEHOLDERS)

file(GLOB SRC src/*.cpp)
file(GLOB HEADERS src/*.h)

add_library(lib ${SRC} ${HEADERS})
target_include_directories(lib PUBLIC "D:/Bibliotheken/boost_1_76_0")
target_link_directories(lib PUBLIC    "D:/Bibliotheken/boost_1_76_0/stage/lib")
target_link_libraries(lib shlwapi.lib)

add_executable(server "src_main/Server.cpp")
add_executable(client "src_main/Client.cpp")
target_link_libraries(server lib)
target_link_libraries(client lib)
