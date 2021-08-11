cmake_minimum_required(VERSION 3.16)

project(Sync)

set(CXX_STANDARD 14)
add_compile_definitions(LIVE)
add_compile_definitions(BIG_ENDIAN)
add_compile_definitions(_WIN32_WINNT=0x0601)
add_compile_definitions(BOOST_BIND_NO_PLACEHOLDERS)

find_package(Threads REQUIRED)

file(GLOB SRC  src/*.cpp)
file(GLOB MAIN src_main/*.cpp)

add_executable(gui ${SRC} ${MAIN})

target_include_directories(gui PUBLIC "D:/Bibliotheken/boost_1_76_0")
target_link_directories(gui PUBLIC    "D:/Bibliotheken/boost_1_76_0/stage/lib")

#target_include_directories(gui PUBLIC ../lib_include/)

#if(WIN32)
#    target_link_directories(gui PUBLIC ../lib_link/win64)
#endif()
#if(LINUX)
#    target_link_directories(gui PUBLIC ../lib_link/linux)
#endif()

#target_link_libraries(gui glfw3 freetype)
#target_link_libraries(gui Threads::Threads)
