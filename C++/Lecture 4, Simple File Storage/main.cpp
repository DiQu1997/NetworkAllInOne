#include "FileStorageServer.h"

#include <iostream>
#include <string>

namespace {
ServerType parseServerType(const std::string& mode) {
    if (mode == "select") {
        return ASYNC_SELECT;
    }

    if (mode == "poll") {
        return ASYNC_POLL;
    }

    if (mode == "epoll") {
        return ASYNC_EPOLL;
    }

    return SYNC;
}
} // namespace

int main(int argc, char* argv[]) {
    int port = 8080;
    std::string mode = "sync";
    std::string storagePath = "./storage";

    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    if (argc > 2) {
        mode = argv[2];
    }

    if (argc > 3) {
        storagePath = argv[3];
    }

    FileStorageServer server(AF_INET, parseServerType(mode), storagePath);

    if (server.server_bind(AF_INET, port) == -1) {
        return 1;
    }

    if (server.server_listen(10) == -1) {
        return 1;
    }

    std::cout << "Listening on port " << port << " with mode " << mode
              << " and storage path " << storagePath << std::endl;

    return server.start();
}
