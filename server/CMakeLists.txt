cmake_minimum_required(VERSION 3.15)
project(server)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS -pthread)
add_executable(server main.cpp DieWithError.cpp HandleTcpClient.cpp ProcessRequest.cpp)
