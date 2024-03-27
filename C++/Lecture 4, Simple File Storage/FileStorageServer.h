#include "../Lecture 3, OOP Refactor/TCPServer.h"
#include <fstream>
#include <iostream>
#include <string>

class FileStorageServer {
public:
    FileStorageServer(int domain, ServerType serverType = SYNC, std::string storagePath = "./");
    ~FileStorageServer();
private:
    TCPServer server_;
    std::string storagePath_;
    std::string getFile(std::string request);
    std::string putFile(std::string request);
    std::string deleteFile(std::string request);
};