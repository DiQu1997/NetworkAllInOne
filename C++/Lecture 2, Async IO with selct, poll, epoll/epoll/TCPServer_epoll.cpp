// TCP Server using epoll
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#define PORT 8080
#define MAX_EVENTS 10 // Max number of events to be returned from a single epoll_wait call

/*
 * Creating an epoll instance: You create an instance of epoll using epoll_create1(0). 
 * This instance will be used to manage multiple file descriptors.
 * 
 * Adding file descriptors: Use epoll_ctl() to add, modify, or remove file descriptors 
 * from the epoll instance. In this example, the server socket is added to listen for 
 * new connections, and new client sockets are added as they connect.
 *
 * Waiting for events: The epoll_wait() call waits for events on the file descriptors 
 * that are being monitored by the epoll instance. It can return multiple events, making 
 * it more efficient than select or poll for large numbers of connections.
 * 
 * Handling events: Based on the events returned by epoll_wait(), you perform the 
 * appropriate I/O operations, such as accepting new connections or reading/writing data.
*/
int main() {
    int server_fd, new_socket, epoll_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Create an epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    // Add the server socket to the epoll event loop
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd");
        exit(EXIT_FAILURE);
    }

    // Event loop
    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                // Handle new connection
                new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
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
                ssize_t count = read(events[n].data.fd, buffer, sizeof(buffer));
                if (count == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                } else if (count == 0) {
                    // End of file. The remote has closed the connection.
                    close(events[n].data.fd);
                } else {
                    // Echo back the received data
                    write(events[n].data.fd, buffer, count);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
