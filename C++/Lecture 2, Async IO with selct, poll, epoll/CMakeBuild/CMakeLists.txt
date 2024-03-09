cmake_minimum_required(VERSION 3.10)
project(AsyncNetwork)

# TCP Server with select
add_executable(TcpServerSelect ../select/TCPServer_select.cpp)

# TCP Server with poll
add_executable(TcpServerPoll ../poll/TCPServer_poll.cpp)

# TCP Client with select
add_executable(TcpClientSelect ../select/TCPClient_select.cpp)

# TCP Server with poll
add_executable(TcpClientPoll ../poll/TCPClient_poll.cpp)