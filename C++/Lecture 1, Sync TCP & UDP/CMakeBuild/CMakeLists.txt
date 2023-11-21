cmake_minimum_required(VERSION 3.10)
project(SyncTCP)

# TCP Server
add_executable(tcp_server ../TCP/SyncTcpServer.cpp)

# TCP Client
add_executable(tcp_client ../TCP/SyncTcpClient.cpp)