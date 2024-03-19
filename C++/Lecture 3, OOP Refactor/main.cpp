#include "TCPServer.h"

int main() {
    TCPServer server(AF_INET, SYNC);
    server.server_bind(AF_INET, 8080);
    server.server_listen(5);
    server.server_accept();

    
    return 0;
}