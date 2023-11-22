#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>

int main() {
    // --- UDP Server Outline ---
    // 1. Create a UDP socket with socket()
    // 2. Define the Server Address (IP Address and Port Number) with sockaddr_in
    // 3. Bind the socket to the address with bind()
    // 4. Receive data with recvfrom()
    // 5. Send data with sendto()
    // 6. Close the socket with close()

    // 1. Create a UDP socket with socket()
    // AF_INET: IPv4, SOCK_DGRAM: UDP
    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create a socket." << std::endl;
        return -1;
    }

    // 2. Define the Server Address (IP Address and Port Number) with sockaddr_in
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // Clear the structure
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to any address
    server_addr.sin_port = htons(8080); // Port Number

    // 3. Bind the socket to the address with bind()
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind the socket to the address." << std::endl;
        close(server_fd);
        return -1;
    }

    // 4. Receive data with recvfrom()
    char buffer[1024] = {0};
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int valread = recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &client_addr_len);
    if (valread == -1) {
        std::cerr << "Failed to receive data from the client." << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Message received from the client: " << buffer << std::endl;

    // 5. Send data with sendto()
    int valsend = sendto(server_fd, buffer, valread, 0, (struct sockaddr*)&client_addr, client_addr_len);
    if (valsend == -1) {
        std::cerr << "Failed to send data to the client." << std::endl;
        close(server_fd);
        return -1;
    }

    // 6. Close the socket with close()
    close(server_fd);
    return 0;
}