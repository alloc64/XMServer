cmake_minimum_required(VERSION 3.22)
project(XMServer)

set(CMAKE_CXX_STANDARD 20)

add_executable(XMServer
        main.cpp
        main.h
        math/vector.cpp
        math/vector.h
        core/md5.h
        core/server.cpp
        core/server.h
        core/xmlevel.cpp
        core/xmlevel.h)

target_link_libraries(XMServer -lwsock32 -lwinmm)
