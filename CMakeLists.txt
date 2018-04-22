cmake_minimum_required(VERSION 3.10)
project(webserver_skeleton)

set(CMAKE_CXX_STANDARD 11)

include_directories(threadpool)

add_executable(webserver_skeleton
        thread/ex1.cpp
        thread/ex2.cpp
        thread/ex3.cpp
        thread/ex4.cpp
        threadpool/example.cpp
        threadpool/ThreadPool.h
        server.cpp)
