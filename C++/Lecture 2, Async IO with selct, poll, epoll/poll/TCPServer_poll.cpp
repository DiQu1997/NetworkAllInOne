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

/*
 * This is a simple TCP server that uses poll to handle multiple clients.
 * What it does:
 * 1. Create a socket, bind it to the port and start listening.
 * 2. When a new client connects, the server fd has an event as POLLIN, it accepts the connection and adds the client fd to the list of fds.
 * 3. When a client sends data, it reads the data and sends it back to the client.
 * 4. If a client disconnects, it removes the client fd from the list of fds.
 * 
 * The event that poll can handle are:
 *      POLLIN: There is data to read.
 *      POLLOUT: Data can be written. Usually happen when the socket is non-blocking.
 *      POLLERR: There is an error.
 *      POLLHUP: The other side has disconnected.
 *      POLLNVAL: The file descriptor is not open.
 * 
 * Comparing to select, poll is more efficient because it doesn't have the limit of 1024 fds and it doesn't have to copy the fd_set every time.
 * However, it still has the same problem as select, it has to loop through all the fds to find the one with an event.
 * And select loop through 0 to max_fd, poll loop through all the fds added
*/

int main() {
    int listener_fd, new_socket, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    std::vector<struct pollfd> fds;

    // 1. Create a socket, bind it to the port and start listening.
    // TODO [Di Qu 03/05/2024] This part as a general server binding section should be extracted to a function
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
                // 2. When a new client connects, the server fd has an event as POLLIN, it accepts the connection and adds the client fd to the list of fds.
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
                    // 3. When a client sends data, it reads the data and sends it back to the client.
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