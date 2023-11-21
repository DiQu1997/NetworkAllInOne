#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

/*
    A simple TCP server that receives a message from a client and sends back the message.
    This server is synchronous, single thread, using blocking socket.
    It can only handle one client at a time.
    It use a simple 1024 bytes buffer to receive and send data.
*/
int main() {
    // --- TCP Server Outline ---
    // 1. Create a socket with socket()
    // 2. Define the Server Address (IP Address and Port Number) with sockaddr_in
    // 3. Bind the socket to the address with bind()
    // 4. Listen for incoming connections with listen()
    // 5. Accept the connection with accept()
    // 6. Send and Receive data with send() and recv()
    // 7. Close the connection(socket) with close()

    // 1. Create a socket with socket()
    // AF_INET: IPv4, SOCK_STREAM: TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create a socket." << std::endl;
        return -1;
    }

    // 2. Define the Server Address (IP Address and Port Number) with sockaddr_in
    // INADDR_ANY:
    //  - When a socket is bound to INADDR_ANY with a specific port,
    //    it means that the socket can be connected to any interface.
    // What does interface mean here?
    //  - An interface is a kind of virtual or physical network card.
    //    It is a software component that allows a computer to connect to a network.
    // Besides INADDR_ANY, there are other special addresses:
    //  - INADDR_LOOPBACK:
    //    - INADDR_LOOPBACK is a special address that always refers to the local host.
    //    - It is used to establish a connection to the same computer.
    //  - INADDR_BROADCAST:
    //    - INADDR_BROADCAST is a special address that refers to all hosts on the same network.
    //    - It is used to send a message to all hosts on the same network.
    //  - INADDR_NONE:
    //    - INADDR_NONE is a special address that indicates an error.
    //    - It is used to indicate an invalid address.
    //  - When a socket is bound to INADDR_ANY with a port of 0,
    //    the OS will assign a unique port to the socket.
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to any address
    server_addr.sin_port = htons(8080); // Port Number
    
    // 3. Bind the socket to the address with bind()
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind the socket to the address." << std::endl;
        return -1;
    }

    // 4. Listen for incoming connections with listen()
    // The second argument is the maximum number of connections that can be queued.
    // If the number of connections exceeds the maximum number of connections that can be queued,
    // the connection will be refused.
    if (listen(server_fd, 5) == -1) {
        std::cerr << "Failed to listen for incoming connections." << std::endl;
        return -1;
    }

    // 5. Accept the connection with accept()
    int addrlen = sizeof(server_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&server_addr, (socklen_t*)&addrlen);
    if (client_fd == -1) {
        std::cerr << "Failed to accept the connection." << std::endl;
        return -1;
    }

    // 6. Send and Receive data with send() and recv()
    char buffer[1024] = {0};
    int valread = recv(client_fd, buffer, 1024, 0);
    std::cout << "Message received: " << buffer << std::endl;

    // 7. Close the connection(socket) with close()
    std::string response = "Echo: " + std::string(buffer);
    send(client_fd, response.c_str(), response.length(), 0);
    std::cout << "Echo response sent: " << response << std::endl;

    // 8. Close the connection(socket) with close()
    close (client_fd);
    close (server_fd);

    return 0;
}