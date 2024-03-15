#include "TCPServer.h"

TCPServer::TCPServer(int domain) {
    serverSocket_ = socket(domain, SOCK_STREAM, 0);
    if (serverSocket_ == -1) {
        std::cerr << "Failed to create a socket." << std::endl;
        return;
    }

    TCPServer::~TCPServer() {
        close(serverSocket_);
    }

    TCPServer::bind(int domain, int port) {
        struct sockaddr_in server_addr;
        server_addr.sin_family = domain;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(serverSocket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            std::cerr << "Failed to bind the socket to the address." << std::endl;
            return -1;
        }
    }

    TCPServer::listen(int backlog) {
        if (listen(serverSocket_, backlog) == -1) {
            std::cerr << "Failed to listen for incoming connections." << std::endl;
            return -1;
        }
    }

    TCPServer::accept() {
        struct sockaddr_in server_addr;
        int addrlen = sizeof(server_addr);
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&server_addr, (socklen_t*)&addrlen);
        if (clientSocket == -1) {
            std::cerr << "Failed to accept the connection." << std::endl;
            return -1;
        }
        clientSockets_.push_back(clientSocket);
        return clientSocket;
    }

    TCPServer::send(int fd, const char* buffer, int length) {
        send(fd, buffer, length, 0);
    }
}