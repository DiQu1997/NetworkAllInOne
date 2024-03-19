#include "TCPServer.h"
#include <algorithm>

TCPServer::TCPServer(int domain, ServerType serverType) : serverType_(serverType){
    serverSocket_ = socket(domain, SOCK_STREAM, 0);
    if (serverSocket_ == -1) {
        std::cerr << "Failed to create a socket." << std::endl;
        return;
    }
}

TCPServer::~TCPServer() {
    close(serverSocket_);
}

int TCPServer::server_bind(int domain, int port) {
    struct sockaddr_in server_addr;
    server_addr.sin_family = domain;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(serverSocket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind the socket to the address." << std::endl;
        return -1;
    }

    return 0;
}

int TCPServer::server_listen(int backlog) {
    if (listen(serverSocket_, backlog) == -1) {
        std::cerr << "Failed to listen for incoming connections." << std::endl;
        return -1;
    }

    return 0;
}

int TCPServer::server_accept() {
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

int TCPServer::sever_select() {
    set_socket_blocking(serverSocket_, false);
    struct sockaddr_in server_addr;
    int addrlen = sizeof(server_addr);
    while(true) {
        FD_ZERO(&selectFdsSet_);

        FD_SET(serverSocket_, &selectFdsSet_);
        int max_sd = serverSocket_;

        for (auto &socket : clientSockets_) {
            FD_SET(socket, &selectFdsSet_);
            if (socket > max_sd) {
                max_sd = socket;
            }
        }

        int activity = select(max_sd + 1, &selectFdsSet_, nullptr, nullptr, nullptr);
        if (activity < 0 && errno != EINTR) {
            std::cerr << "Select error" << std::endl;
        }

        // If something happened on the master socket, then it's an incoming connection
        if (FD_ISSET(serverSocket_, &selectFdsSet_)) {
            int new_socket = accept(serverSocket_, (struct sockaddr *)&server_addr, (socklen_t*)&addrlen);
            if (new_socket < 0) {
                std::cerr << "Accept error" << std::endl;
                exit(EXIT_FAILURE);
            }
            
            std::cout << "New connection, socket fd is " << new_socket << std::endl;

            // Add new socket to array of sockets
            auto it = std::find(clientSockets_.begin(), clientSockets_.end(), new_socket);
            if (it != clientSockets_.end()) {
                *it = new_socket;
            }
        }

        // Handle IO for each socket
        for (auto &socket : clientSockets_) {
            int sd = socket;

            if (FD_ISSET(sd, &selectFdsSet_)) {
                // Check if it was for closing, and also read the incoming message
                int valread = 0;
                if ((valread = read(sd, buffer_, BUFFER_SIZE)) == 0) {
                    // Somebody disconnected
                    getpeername(sd, (struct sockaddr*)&server_addr, (socklen_t*)&addrlen);
                    std::cout << "Host disconnected, ip " << inet_ntoa(server_addr.sin_addr) << ", port " << ntohs(server_addr.sin_port) << std::endl;
                    
                    // Close the socket and mark as 0 in list for reuse
                    close(sd);
                    socket = 0;
                } else {
                    // Echo back the message that came in
                    buffer_[valread] = '\0';
                    server_send(sd, buffer_, valread);
                }
            }
        }

    }
}

int TCPServer::server_poll() {
    set_socket_blocking(serverSocket_, false);
    struct sockaddr_in server_addr;
    int addrlen = sizeof(server_addr);

    struct pollfd server_pollfd;
    server_pollfd.fd = serverSocket_;
    server_pollfd.events = POLLIN;
    pollFds_.push_back(server_pollfd);

    std::cout << "Waiting for connections..." << std::endl;

    while(true) {
        int ret = poll(pollFds_.data(), pollFds_.size(), -1);

        if (ret < 0) {
            std::cerr << "Poll failed." << std::endl;
            return -1;
        }
        
        int valread = 0;

        for (int i = 0; i < pollFds_.size(); i++) {
            // find a fd with an event as POLLIN, means there is data to read
            if (pollFds_[i].revents & POLLIN) {
                // 2. When a new client connects, the server fd has an event as POLLIN, it accepts the connection and adds the client fd to the list of fds.
                int new_socket;
                if (pollFds_[i].fd == serverSocket_) {
                    if ((new_socket = accept(serverSocket_, (struct sockaddr *)&server_addr, (socklen_t*)&addrlen)) < 0) {
                        std::cerr << "Failed to accept a connection." << std::endl;
                        return -1;
                    }

                    struct pollfd client_fd;
                    client_fd.fd = new_socket;
                    client_fd.events = POLLIN;
                    pollFds_.push_back(client_fd);

                    std::cout << "New connection, socket fd is " << new_socket << ", ip is " << inet_ntoa(server_addr.sin_addr) << ", port: " << ntohs(server_addr.sin_port) << std::endl;
                } else {
                    // 3. When a client sends data, it reads the data and sends it back to the client.
                    valread = read(pollFds_[i].fd, buffer_, BUFFER_SIZE);
                    if (valread == 0) {
                        std::cout << "Host disconnected." << std::endl;
                        close(pollFds_[i].fd);
                        pollFds_.erase(pollFds_.begin() + i);
                    } else {
                        send(pollFds_[i].fd, buffer_, valread, 0);
                    }
                }
            }
        }
    }
}

int TCPServer::server_epoll(const int max_events) {
    set_socket_blocking(serverSocket_, false);
    struct sockaddr_in server_addr;
    int addrlen = sizeof(server_addr);

    // Create an epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    struct epoll_event events[max_events];
    ev.events = EPOLLIN;
    ev.data.fd = serverSocket_;

    // Add the server socket to the epoll event loop
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket_, &ev) == -1) {
        perror("epoll_ctl: server_fd");
        exit(EXIT_FAILURE);
    }

    // Event loop
    while (true) {
        int nfds = epoll_wait(epoll_fd, events, max_events, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == serverSocket_) {
                // Handle new connection
                int new_socket = accept(serverSocket_, (struct sockaddr *)&server_addr, (socklen_t*)&addrlen);
                if (new_socket == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                ev.events = EPOLLIN;
                ev.data.fd = new_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev) == -1) {
                    perror("epoll_ctl: new_socket");
                    exit(EXIT_FAILURE);
                }
            } else {
                // Handle data from a client
                ssize_t count = read(events[n].data.fd, buffer_, sizeof(buffer_));
                if (count == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                } else if (count == 0) {
                    // End of file. The remote has closed the connection.
                    close(events[n].data.fd);
                } else {
                    // Echo back the received data
                    server_send(events[n].data.fd, buffer_, count);
                }
            }
        }
    }
}

int TCPServer::server_send(int fd, const char* buffer, int length) {
    return send(fd, buffer, length, 0);
} 