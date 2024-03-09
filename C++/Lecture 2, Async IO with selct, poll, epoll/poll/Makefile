CC=g++
CFLAGS=-I.

all : tcp_server_poll tcp_client_poll

tcp_server_poll : TCPServer_poll.cpp
	$(CC) -o TCPServer_poll TCPServer_poll.cpp $(CFLAGS)

tcp_client_poll : TCPClient_poll.cpp
	$(CC) -o TCPClient_poll TCPClient_poll.cpp $(CFLAGS)

clean :
	rm TCPServer_poll TCPClient_poll
