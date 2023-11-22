CC=g++
CFLAGS=-I.

all: tcp udp

tcp: SyncTCPServer SyncTCPClient

udp: SyncUDPServer SyncUDPClient

SyncTCPServer: TCP/SyncTcpServer.cpp
	$(CC) -o SyncTcpServer TCP/SyncTcpServer.cpp $(CFLAGS)

SyncTCPClient: TCP/SyncTcpClient.cpp
	$(CC) -o SyncTcpClient TCP/SyncTcpClient.cpp $(CFLAGS)

SyncUDPServer: UDP/SyncUdpServer.cpp
	$(CC) -o SyncUdpServer UDP/SyncUdpServer.cpp $(CFLAGS)

SyncUDPClient: UDP/SyncUdpClient.cpp
	$(CC) -o SyncUdpClient UDP/SyncUdpClient.cpp $(CFLAGS)

clean:
	rm -f SyncTcpServer SyncTcpClient SyncUdpServer SyncUdpClient