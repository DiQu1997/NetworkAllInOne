#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

const int PORT = 8080;
const char* SERVER_IP = "127.0.0.1";
const int BUFFER_SIZE = 1024;

int set_socket_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    flags |= O_NONBLOCK;
    int s = fcntl(sockfd, F_SETFL, flags);
    if (s == -1) {
        return -1;
    }
    return 0;
}

int main () {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create a socket." << std::endl;
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported." << std::endl;
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed." << std::endl;
        close(sockfd);
        return -1;
    }

    const char* message = "Hello from client!";
    send(sockfd, message, strlen(message), 0);
    std::cout << "Message sent." << std::endl;

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int valread = read(sockfd, buffer, BUFFER_SIZE);
    if (valread < 0) {
        std::cerr << "Failed to read from socket." << std::endl;
        close(sockfd);
        return -1;
    }

    std::cout << "Message from server: " << buffer << std::endl;
}