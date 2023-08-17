cmake_minimum_required(VERSION 3.25)
project(hand_async)

set(CMAKE_CXX_STANDARD 20)

add_executable(hand_async main.cpp async/EventLoop.hpp async/EventLoop.cpp async/IClient.cpp)
