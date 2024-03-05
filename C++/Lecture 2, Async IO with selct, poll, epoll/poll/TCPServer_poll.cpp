#include <iostream>
#include <vector>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 30

int main() {
    int listener_fd, new_socket, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    std::vector<struct pollfd> fds;

    // Create a socket
    if((listener_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Failed to create a socket." << std::endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the port
    if (bind(listener_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind the socket to the port." << std::endl;
        return -1;
    }

    if (listen(listener_fd, 3) < 0) {
        std::cerr << "Failed to listen." << std::endl;
        return -1;
    }

    struct pollfd listener;
    listener.fd = listener_fd;
    listener.events = POLLIN;
    fds.push_back(listener);

    std::cout << "Waiting for connections..." << std::endl;

    while (true) {
        // check events:
        int ret = poll(fds.data(), fds.size(), -1);

        if (ret < 0) {
            std::cerr << "Poll failed." << std::endl;
            return -1;
        }

        for (int i = 0; i < fds.size(); i++) {
            // find a fd with an event as POLLIN, means there is data to read
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == listener_fd) {
                    if ((new_socket = accept(listener_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                        std::cerr << "Failed to accept a connection." << std::endl;
                        return -1;
                    }

                    struct pollfd client_fd;
                    client_fd.fd = new_socket;
                    client_fd.events = POLLIN;
                    fds.push_back(client_fd);

                    std::cout << "New connection, socket fd is " << new_socket << ", ip is " << inet_ntoa(address.sin_addr) << ", port: " << ntohs(address.sin_port) << std::endl;
                } else {
                    // We have data from a client
                    valread = read(fds[i].fd, buffer, BUFFER_SIZE);
                    if (valread == 0) {
                        std::cout << "Host disconnected." << std::endl;
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                    } else {
                        send(fds[i].fd, buffer, valread, 0);
                    }
                }
            }
        }
    }
    return 0;
}