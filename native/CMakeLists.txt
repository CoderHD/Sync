cmake_minimum_required(VERSION 3.16)

project(Sync)

set(CXX_STANDARD 14)
add_compile_definitions(LIVE)
add_compile_definitions(BIG_ENDIAN)
add_compile_definitions(_WIN32_WINNT=0x0601)
add_compile_definitions(BOOST_BIND_NO_PLACEHOLDERS)

file(GLOB SRC src/*.cpp)
file(GLOB HEADERS src/*.h)

add_library(lib ${SRC} ${HEADERS})
target_include_directories(lib PUBLIC "D:/Bibliotheken/boost_1_76_0")
target_link_directories(lib PUBLIC    "D:/Bibliotheken/boost_1_76_0/stage/lib")

add_executable(server "src_main/Server.cpp")
add_executable(client "src_main/Client.cpp")
target_link_libraries(server lib)
target_link_libraries(client lib)
