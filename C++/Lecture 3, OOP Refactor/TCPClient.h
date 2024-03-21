#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unordered_map>

class TCPClient {

public:
    TCPClient();
    ~TCPClient();

    int client_connect(const char* ip, int port);
    int client_send(const char* message, int size);
    int client_receive(char* buffer, int size);
    void client_close();

private:
    int clientSocket_;
    std::unordered_map<int , struct sockaddr_in> serverAddress_;
    char buffer_[1024];
};