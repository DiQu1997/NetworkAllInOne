#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MAX_EVENTS 10 // Max number of events to be returned from a single epoll_wait call

// Function to set a socket to non-blocking mode
void set_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL) failed");
        exit(EXIT_FAILURE);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL) failed");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    set_non_blocking(sockfd);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        if (errno != EINPROGRESS) {
            perror("connect failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1 failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLOUT | EPOLLIN | EPOLLET; // Interested in write and read events, edge-triggered
    ev.data.fd = sockfd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        perror("epoll_ctl failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Wait for the socket to be ready
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds < 0) {
        perror("epoll_wait failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < nfds; i++) {
        if (events[i].events & EPOLLOUT) {
            // The socket is writable now, send the message
            const char* message = "Hello, Server!";
            send(events[i].data.fd, message, strlen(message), 0);
            ev.events = EPOLLIN | EPOLLET; // Now wait for response
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &ev);
        }
        if (events[i].events & EPOLLIN) {
            char buffer[1024];
            int bytes_read = read(events[i].data.fd, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                std::cout << "Received from server: " << buffer << std::endl;
            }
        }
    }

    // Check for reply
    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds < 0) {
        perror("epoll_wait failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }


    for (int i = 0; i < nfds; i++) {
        if (events[i].events & EPOLLOUT) {
            // The socket is writable now, send the message
            const char* message = "Hello, Server!";
            send(events[i].data.fd, message, strlen(message), 0);
            ev.events = EPOLLIN | EPOLLET; // Now wait for response
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &ev);
        }
        if (events[i].events & EPOLLIN) {
            char buffer[1024];
            int bytes_read = read(events[i].data.fd, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                std::cout << "Received from server: " << buffer << std::endl;
            }
        }
    }

    close(sockfd);
    return 0;
}