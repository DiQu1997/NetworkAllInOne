#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h> // for close

int main() {
    // --- UDP Client Outline ---
    // 1. Create a UDP socket.
    // 2. Set up the server address and port.
    // 3. Send a message to the server.
    // 4. Optionally, receive a response from the server.
    // 5. Close the socket.

    // 1. Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error opening socket." << std::endl;
        return 1;
    }

    // 2. Set up the server address and port
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr)); // Clear structure
    servaddr.sin_family = AF_INET;          // IPv4
    servaddr.sin_port = htons(8080);        // Port number
    if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0) {
        std::cerr << "Error in inet_pton." << std::endl;
        close(sockfd);
        return 1;
    }

    // 3. Send a message to the server
    const char *message = "Hello, UDP Server!";
    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "Error in sendto." << std::endl;
        close(sockfd);
        return 1;
    }

    // 4. Optionally, receive a response from the server (omitted in this example)
    char buffer[1024] = {0};
    socklen_t servaddr_len = sizeof(servaddr);
    int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&servaddr, &servaddr_len);
    if (n < 0) {
        std::cerr << "Error in recvfrom." << std::endl;
        close(sockfd);
        return 1;
    }

    // 5. Close the socket
    close(sockfd);
    return 0;
}
