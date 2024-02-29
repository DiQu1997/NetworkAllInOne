#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;
/*
 * A simple TCP server that listens to a port and accepts multiple clients using select().
 * This server is synchronous, single thread, using blocking socket.
 * The key data structure for select() is fd_set, which is a bit array.
 * It is used to tell the kernel which file descriptors we are interested in.
 * The server can handle multiple clients at the same time by using select() to monitor multiple file descriptors.
 ************************************************************************
 * How select works:
 * 1. Create a master socket, bind it to the port, and start listening. This is the server socket fd.
 * 2. Create a set of socket fds (fd_set) to monitor. Add the server socket fd to the set.
 * 3. Use select() to monitor the set of socket fds. It will block until there is an activity on one of the fds.
 * 4. If the server socket fd has activity, it means there is a new client connection. Accept the new connection and add the new socket fd to the set.
 * 5. If a client socket fd has activity, it means there is data to read. Read the data and send back a response.
*/

// It is a bit array where each bit represents a file descriptor
// It is used to tell the kernel which file descriptors we are interested in



int main() {
    
    int master_socket, addrlen, new_socket, max_clients = 30, activity, valread, sd;
    int max_sd;
    struct sockaddr_in address;
    std::vector<int> client_sockets(max_clients, 0); // Initialize all to 0
    char buffer[BUFFER_SIZE]; // Data buffer
    fd_set readfds;

    // Create a master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Failed to create a socket." << std::endl;
        return -1;
    }

    // Set master socket options
    // This is to avoid "Address already in use" error when restarting the server
    // SOL_SOCKET: Socket level, it means we are configuring the socket itself
    // SO_REUSEADDR: Allows other sockets to bind() to this port, unless there is an active listening socket bound to the port already
    int opt = 1;
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set master socket options." << std::endl;
        return -1;
    }

    // Set the socket to bind to all available interfaces and listen to the specified port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Listener on port " << PORT << std::endl;
        
    if (listen(master_socket, 3) < 0) {
        std::cerr << "Listen error" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Accept incoming connections
    addrlen = sizeof(address);
    std::cout << "Waiting for connections..." << std::endl;

    while (true) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        // Add child sockets to set
        for (auto &socket : client_sockets) {
            sd = socket;
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Wait for an activity on one of the sockets
        activity = select(max_sd + 1, &readfds, nullptr, nullptr, nullptr);

        if (activity < 0 && errno != EINTR) {
            std::cerr << "Select error" << std::endl;
        }

        // If something happened on the master socket, then it's an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                std::cerr << "Accept error" << std::endl;
                exit(EXIT_FAILURE);
            }
            
            std::cout << "New connection, socket fd is " << new_socket << std::endl;

            // Add new socket to array of sockets
            auto it = std::find(client_sockets.begin(), client_sockets.end(), 0);
            if (it != client_sockets.end()) {
                *it = new_socket;
            }
        }

        // Handle IO for each socket
        for (auto &socket : client_sockets) {
            sd = socket;

            if (FD_ISSET(sd, &readfds)) {
                // Check if it was for closing, and also read the incoming message
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
                    // Somebody disconnected
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    std::cout << "Host disconnected, ip " << inet_ntoa(address.sin_addr) << ", port " << ntohs(address.sin_port) << std::endl;
                    
                    // Close the socket and mark as 0 in list for reuse
                    close(sd);
                    socket = 0;
                } else {
                    // Echo back the message that came in
                    buffer[valread] = '\0';
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    }

}