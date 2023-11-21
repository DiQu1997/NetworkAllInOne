#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

/*
    A simple TCP client that sends a message to a server and receives back the message.
    This client is synchronous, single thread, using blocking socket.
    It can only handle one server at a time.
    It use a simple 1024 bytes buffer to receive and send data.
*/

int main() {
    // --- TCP Client Outline ---
    // 1. Create a socket with socket()
    // 2. Define the Server Address (IP Address and Port Number) with sockaddr_in
    // 3. Connect to the server with connect()
    // 4. Send and Receive data with send() and recv()
    // 5. Close the connection(socket) with close()

    // 1. Create a socket with socket()
    // AF_INET: IPv4, SOCK_STREAM: TCP
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create a socket." << std::endl;
        return -1;
    }

    // 2. Define the Server Address (IP Address and Port Number) with sockaddr_in
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_port = htons(8080); // Port 8080
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr); // Server IP

    // 3. Connect to the server with connect()
    if (connect(sock , (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        std::cerr << "Failed to connect to the server." << std::endl;
        return -1;
    }

    // 4. Send and Receive data with send() and recv()
    std::string message = "Hello World!";
    send(sock, message.c_str(), message.size(), 0);
    std::cout << "Message sent to the server: " << message << std::endl;

    char buffer[1024] = {0};
    int valread = recv(sock, buffer, 1024, 0);
    std::cout << "Server response: " << buffer << std::endl;

    // 5. Close the connection(socket) with close()
    close(sock);

    return 0;
}