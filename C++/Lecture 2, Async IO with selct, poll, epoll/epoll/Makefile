CC=g++
CFLAGS=-I.

all : tcp_server_epoll tcp_client_epoll

tcp_server_epoll : TCPServer_epoll.cpp
	$(CC) -o TCPServer_epoll TCPServer_epoll.cpp $(CFLAGS)

tcp_client_epoll : TCPClient_epoll.cpp
	$(CC) -o TCPClient_epoll TCPClient_epoll.cpp $(CFLAGS)
	
clean :
	rm TCPServer_epoll
