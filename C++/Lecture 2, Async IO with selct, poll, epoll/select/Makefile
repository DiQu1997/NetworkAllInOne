CC=g++
CFLAGS=-I.

all : tcp_server_select tcp_client_select

tcp_server_select : TCPServer_select.cpp
	$(CC) -o TCPServer_select TCPServer_select.cpp $(CFLAGS)

tcp_client_select : TCPClient_select.cpp
	$(CC) -o TCPClient_select TCPClient_select.cpp $(CFLAGS)

clean :
	rm TCPServer_select TCPClient_select
