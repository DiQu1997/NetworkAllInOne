#include "FileStorageServer.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

FileStorageServer::FileStorageServer(int domain, ServerType serverType, std::string storagePath)
    : serverSocket_(-1), serverType_(serverType), storagePath_(std::move(storagePath)) {
    serverSocket_ = socket(domain, SOCK_STREAM, 0);
    if (serverSocket_ == -1) {
        std::cerr << "Failed to create a socket." << std::endl;
        return;
    }

    int enable = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    std::error_code errorCode;
    std::filesystem::create_directories(storagePath_, errorCode);
}

FileStorageServer::~FileStorageServer() {
    for (int clientSocket : clientSockets_) {
        close(clientSocket);
    }

    if (serverSocket_ != -1) {
        close(serverSocket_);
    }
}

int FileStorageServer::server_bind(int domain, int port) {
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = domain;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Failed to bind the socket to the address." << std::endl;
        return -1;
    }

    return 0;
}

int FileStorageServer::server_listen(int backlog) {
    if (listen(serverSocket_, backlog) == -1) {
        std::cerr << "Failed to listen for incoming connections." << std::endl;
        return -1;
    }

    return 0;
}

int FileStorageServer::start(int maxEvents) {
    switch (serverType_) {
        case SYNC:
            return runSync();
        case ASYNC_SELECT:
            return runSelect();
        case ASYNC_POLL:
            return runPoll();
        case ASYNC_EPOLL:
            return runEpoll(maxEvents);
    }

    return -1;
}

// Read one complete request from the client socket.
//
// GET and DELETE requests always fit in a single recv() call because they carry
// no payload. PUT requests may span multiple TCP segments — the content_length
// field tells us exactly how many payload bytes to wait for, so we keep calling
// recv() until the full content arrives before returning.
//
// Wire layout for PUT (spaces are literal field separators):
//   "PUT " <filepath:str> " " <content_length:decimal> " " <content:bytes>
//    [0-3]  [4 .. fpe-1]      [fpe+1 .. fle-1]              [fle+1 ..]
//
//   fpe = index of the space after filepath   (filePathEnd)
//   fle = index of the space after the length (fileLengthEnd)
std::string FileStorageServer::readRequest(int clientSocket) {
    std::string request;
    char buffer[BUFFER_SIZE];

    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0) {
        return "";
    }

    request.append(buffer, bytesRead);

    // GET / DELETE carry no payload — what we received is the full request.
    if (request.rfind("PUT ", 0) != 0) {
        return request;
    }

    // For PUT: parse the content_length field and keep reading until all
    // content bytes have arrived. This handles large files that arrive in
    // multiple TCP segments.
    size_t filePathEnd = request.find(' ', 4);
    if (filePathEnd == std::string::npos) {
        return request;
    }

    size_t fileLengthEnd = request.find(' ', filePathEnd + 1);
    if (fileLengthEnd == std::string::npos) {
        return request;
    }

    int fileLength = std::stoi(request.substr(filePathEnd + 1, fileLengthEnd - filePathEnd - 1));
    size_t contentStart = fileLengthEnd + 1;

    while (static_cast<int>(request.size() - contentStart) < fileLength) {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            break;
        }
        request.append(buffer, bytesRead);
    }

    return request;
}

std::string FileStorageServer::handleRequest(const std::string& request) {
    if (request.rfind("GET ", 0) == 0) {
        return getFile(request);
    }

    if (request.rfind("PUT ", 0) == 0) {
        return putFile(request);
    }

    if (request.rfind("DELETE ", 0) == 0) {
        return deleteFile(request);
    }

    return "ERROR";
}

// Protocol: "GET <filepath>"
//   request[0..3] = "GET "
//   request[4..]  = filepath (no spaces)
// Response: raw file bytes, or "NOT_FOUND" if the file does not exist.
std::string FileStorageServer::getFile(const std::string& request) {
    std::string filePath = request.substr(4);
    std::ifstream input(resolvePath(filePath), std::ios::binary);
    if (!input.is_open()) {
        return "NOT_FOUND";
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

// Protocol: "PUT <filepath> <content_length> <content>"
//   request[0..3]           = "PUT "
//   request[4..fpe-1]       = filepath (no spaces)
//   request[fpe]            = ' '
//   request[fpe+1..fle-1]   = content_length as a decimal integer
//   request[fle]            = ' '
//   request[fle+1..fle+len] = exactly content_length bytes of file content
// Response: "OK" on success, "ERROR" if the file cannot be opened for writing.
//
// Using an explicit content_length (rather than a delimiter) keeps binary
// files safe — the content bytes are written as-is without any escaping.
std::string FileStorageServer::putFile(const std::string& request) {
    size_t filePathEnd = request.find(' ', 4);
    size_t fileLengthEnd = request.find(' ', filePathEnd + 1);

    std::string filePath = request.substr(4, filePathEnd - 4);
    int fileLength = std::stoi(request.substr(filePathEnd + 1, fileLengthEnd - filePathEnd - 1));
    std::string fileContent = request.substr(fileLengthEnd + 1, fileLength);

    std::ofstream output(resolvePath(filePath), std::ios::binary);
    if (!output.is_open()) {
        return "ERROR";
    }

    output.write(fileContent.data(), fileContent.size());
    return output.good() ? "OK" : "ERROR";
}

// Protocol: "DELETE <filepath>"
//   request[0..6] = "DELETE "
//   request[7..]  = filepath (no spaces)
// Response: "OK" on success, "ERROR" if std::remove() fails (e.g., file not found).
std::string FileStorageServer::deleteFile(const std::string& request) {
    std::string filePath = request.substr(7);
    return std::remove(resolvePath(filePath).c_str()) == 0 ? "OK" : "ERROR";
}

std::string FileStorageServer::resolvePath(const std::string& filePath) const {
    if (storagePath_.empty() || storagePath_ == ".") {
        return filePath;
    }

    if (storagePath_.back() == '/') {
        return storagePath_ + filePath;
    }

    return storagePath_ + "/" + filePath;
}

int FileStorageServer::sendAll(int clientSocket, const std::string& response) {
    size_t totalSent = 0;

    while (totalSent < response.size()) {
        ssize_t sent = send(clientSocket, response.data() + totalSent, response.size() - totalSent, 0);
        if (sent <= 0) {
            return -1;
        }
        totalSent += static_cast<size_t>(sent);
    }

    return 0;
}

void FileStorageServer::closeClient(int clientSocket) {
    auto it = std::find(clientSockets_.begin(), clientSockets_.end(), clientSocket);
    if (it != clientSockets_.end()) {
        clientSockets_.erase(it);
    }

    close(clientSocket);
}

int FileStorageServer::runSync() {
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &addrLen);
        if (clientSocket == -1) {
            std::cerr << "Failed to accept the connection." << std::endl;
            continue;
        }

        clientSockets_.push_back(clientSocket);
        std::string request = readRequest(clientSocket);
        if (!request.empty()) {
            sendAll(clientSocket, handleRequest(request));
        }
        closeClient(clientSocket);
    }
}

int FileStorageServer::runSelect() {
    set_socket_blocking(serverSocket_, false);

    while (true) {
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(serverSocket_, &readFds);

        int maxFd = serverSocket_;
        for (int clientSocket : clientSockets_) {
            FD_SET(clientSocket, &readFds);
            maxFd = std::max(maxFd, clientSocket);
        }

        int activity = select(maxFd + 1, &readFds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "Select failed." << std::endl;
            return -1;
        }

        if (FD_ISSET(serverSocket_, &readFds)) {
            struct sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &addrLen);
            if (clientSocket >= 0) {
                clientSockets_.push_back(clientSocket);
            }
        }

        for (size_t i = 0; i < clientSockets_.size();) {
            int clientSocket = clientSockets_[i];
            if (!FD_ISSET(clientSocket, &readFds)) {
                ++i;
                continue;
            }

            std::string request = readRequest(clientSocket);
            if (!request.empty()) {
                sendAll(clientSocket, handleRequest(request));
            }

            close(clientSocket);
            clientSockets_.erase(clientSockets_.begin() + i);
        }
    }
}

int FileStorageServer::runPoll() {
    set_socket_blocking(serverSocket_, false);
    pollFds_.clear();
    pollFds_.push_back({serverSocket_, POLLIN, 0});

    while (true) {
        int activity = poll(pollFds_.data(), pollFds_.size(), -1);
        if (activity < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "Poll failed." << std::endl;
            return -1;
        }

        for (size_t i = 0; i < pollFds_.size();) {
            short events = pollFds_[i].revents;
            if ((events & (POLLIN | POLLHUP | POLLERR)) == 0) {
                ++i;
                continue;
            }

            if (pollFds_[i].fd == serverSocket_) {
                struct sockaddr_in clientAddr;
                socklen_t addrLen = sizeof(clientAddr);
                int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &addrLen);
                if (clientSocket >= 0) {
                    clientSockets_.push_back(clientSocket);
                    pollFds_.push_back({clientSocket, POLLIN, 0});
                }
                ++i;
                continue;
            }

            int clientSocket = pollFds_[i].fd;
            std::string request = readRequest(clientSocket);
            if (!request.empty()) {
                sendAll(clientSocket, handleRequest(request));
            }

            closeClient(clientSocket);
            pollFds_.erase(pollFds_.begin() + i);
        }
    }
}

int FileStorageServer::runEpoll(int maxEvents) {
#ifdef __linux__
    set_socket_blocking(serverSocket_, false);

    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        std::cerr << "Failed to create epoll instance." << std::endl;
        return -1;
    }

    std::vector<struct epoll_event> events(maxEvents);
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = serverSocket_;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket_, &event) == -1) {
        std::cerr << "Failed to add server socket to epoll." << std::endl;
        close(epollFd);
        return -1;
    }

    while (true) {
        int ready = epoll_wait(epollFd, events.data(), maxEvents, -1);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "epoll_wait failed." << std::endl;
            close(epollFd);
            return -1;
        }

        for (int i = 0; i < ready; ++i) {
            int fd = events[i].data.fd;

            if (fd == serverSocket_) {
                struct sockaddr_in clientAddr;
                socklen_t addrLen = sizeof(clientAddr);
                int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &addrLen);
                if (clientSocket == -1) {
                    continue;
                }

                clientSockets_.push_back(clientSocket);

                struct epoll_event clientEvent;
                clientEvent.events = EPOLLIN;
                clientEvent.data.fd = clientSocket;
                epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &clientEvent);
                continue;
            }

            std::string request = readRequest(fd);
            if (!request.empty()) {
                sendAll(fd, handleRequest(request));
            }

            epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
            closeClient(fd);
        }
    }
#else
    (void)maxEvents;
    std::cerr << "epoll is only supported on Linux." << std::endl;
    return -1;
#endif
}
