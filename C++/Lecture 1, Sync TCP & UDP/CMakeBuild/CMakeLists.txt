cmake_minimum_required(VERSION 3.10)
project(SyncNetwork)

# TCP Server
add_executable(SyncTCPServer ../TCP/SyncTcpServer.cpp)

# TCP Client
add_executable(SyncTCPClient ../TCP/SyncTcpClient.cpp)

# UDP Server
add_executable(SyncUDPServer ../UDP/SyncUdpServer.cpp)

# UDP Client
add_executable(SyncUDPClient ../UDP/SyncUdpClient.cpp)