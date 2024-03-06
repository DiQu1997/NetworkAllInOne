#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <iostream>
#include <fcntl.h>
#include <string.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

/**
 * This is a simple TCP client that will send request to the server and print the response.
 * What it does:
 * 1. Create a socket and connect to the server.
 * 2. Send a request to the server.
 * 3. Print the response from the server.
*/

// Set a file descriptor to non-blocking mode
int make_socket_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "fcntl F_GETFL failed" << std::endl;
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        std::cerr << "fcntl F_SETFL failed" << std::endl;
        return -1;
    }
    return 0;
}

int main () {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // 1. Create a socket and connect to the server.
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Failed to create a socket." << std::endl;
        return -1;
    }

    make_socket_non_blocking(sock);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        if (errno != EINPROGRESS) {
            std::cerr << "Failed to connect to the server." << std::endl;
            return -1;
        }
    }

    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLOUT;

    int ret = poll(fds, 1, 10000);
    if (ret < 0) {
        std::cerr << "poll failed" << std::endl;
        return -1;
    } else if (ret == 0) {
        std::cerr << "poll timeout" << std::endl;
        return -1;
    } else {
        if (fds[0].revents & POLLOUT) {
            std::cout << "Connected to the server." << std::endl;
        } else {
            std::cerr << "Failed to connect to the server." << std::endl;
            return -1;
        }
    }

    if (fds[0].revents & POLLOUT) {
        int error;
        socklen_t len = sizeof(error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            std::cerr << "getsockopt failed" << std::endl;
            return -1;
        }
        
        // Check if the connection is successful
        const char* message = "Hello from client!";
        send(sock, message, strlen(message), 0);
        std::cout << "Message sent." << std::endl;

        //Wait for the response from the server
        fds[0].events = POLLIN; // Wait for the socket to be readable
        ret = poll(&fds[0], 1, 5000); // 5 second timeout
        if (ret > 0 && (fds[0].revents & POLLIN)) {
            char buffer[BUFFER_SIZE];
            int bytesReceived = read(sock, buffer, BUFFER_SIZE);
            if (bytesReceived > 0) {
                std::cout << "Response from server: " << buffer << std::endl;
            }
        } else {
            std::cerr << "No response from server\n";
        }
    }

    close(sock);
    return 0;
}