#include <vector>
/*
 * A general class for a TCP server
 * It includes:
 * 1. Initialize the server with socket
 * 2. Bind 
 * 3. Listen
 * 4. Accept (sync accept or assync with select / poll / epoll)
*/

class TCPServer {
public:
    TCPServer(int domain);
    ~TCPServer();

    int bind(int domain, int port);
    int listen(int backlog);

    int accept();
    int send(int fd, const char* buffer, int length);

private:
    int server_socket;
    std::vector<int> client_sockets;
};