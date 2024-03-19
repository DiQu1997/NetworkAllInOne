#include <vector>
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <poll.h>
#include <sys/epoll.h>

#pragma once

/*
 * A general class for a TCP server
 * It includes:
 * 1. Initialize the server with socket
 * 2. Bind 
 * 3. Listen
 * 4. Accept (sync accept or assync with select / poll / epoll)
*/

#define BUFFER_SIZE 1024

enum ServerType {
    SYNC,
    ASYNC_SELECT,
    ASYNC_POLL,
    ASYNC_EPOLL
};

int set_socket_blocking(int sockfd, bool blocking) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    int s = fcntl(sockfd, F_SETFL, flags);
    if (s == -1) {
        return -1;
    }
    return 0;
}

class TCPServer {
public:
    TCPServer(int domain, ServerType serverType = SYNC);
    ~TCPServer();

    int server_bind(int domain, int port);
    int server_listen(int backlog);

    // Handler functions
    int server_accept();
    int sever_select();
    int server_poll();
    int server_epoll(int max_events);

    int server_send(int fd, const char* buffer, int length);

private:
    int serverSocket_;
    std::vector<int> clientSockets_;
    ServerType serverType_;
    char buffer_[BUFFER_SIZE];

    // Select
    fd_set selectFdsSet_;
    int maxFd_;

    // Poll
    std::vector<struct pollfd> pollFds_;
};