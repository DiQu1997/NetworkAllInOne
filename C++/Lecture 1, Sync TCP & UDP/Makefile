CC=g++
CFLAGS=-I.

all: tcp

tcp: tcp_server tcp_client

tcp_server: TCP/SyncTcpServer.cpp
	$(CC) -o SyncTcpServer TCP/SyncTcpServer.cpp $(CFLAGS)

tcp_client: TCP/SyncTcpClient.cpp
	$(CC) -o SyncTcpClient TCP/SyncTcpClient.cpp $(CFLAGS)

clean:
	rm -f SyncTcpServer SyncTcpClient