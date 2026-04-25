#ifndef FILE_STORAGE_SERVER_H
#define FILE_STORAGE_SERVER_H

#include "../Lecture 3, OOP Refactor/TCPServer.h"
#include <poll.h>
#include <string>
#include <vector>

/*
 * File Storage Protocol (Version 1)
 * ===================================
 * A simple plain-text protocol over TCP. Each request is one logical message;
 * each response is one logical message. The connection is closed after the
 * server sends its response (request-response, no pipelining).
 *
 * Commands
 * --------
 *
 * GET <filepath>
 *   Retrieve a file from the server.
 *   <filepath>  — path relative to the server's storagePath, no spaces allowed.
 *   Response    — raw file bytes on success, or the literal string "NOT_FOUND".
 *
 *   Example:
 *     Client → "GET notes/todo.txt"
 *     Server → <contents of notes/todo.txt>   (or "NOT_FOUND")
 *
 * PUT <filepath> <content_length> <content>
 *   Store a file on the server. The content is binary-safe because its length
 *   is declared upfront rather than delimited by a special character.
 *   <filepath>       — path relative to storagePath, no spaces allowed.
 *   <content_length> — decimal byte count of <content>.
 *   <content>        — exactly <content_length> bytes of file data.
 *   Response         — "OK" on success, "ERROR" on failure.
 *
 *   Example:
 *     Client → "PUT notes/todo.txt 13 Hello, World!"
 *     Server → "OK"
 *
 * DELETE <filepath>
 *   Remove a file from the server.
 *   <filepath>  — path relative to storagePath, no spaces allowed.
 *   Response    — "OK" on success, "ERROR" on failure.
 *
 *   Example:
 *     Client → "DELETE notes/todo.txt"
 *     Server → "OK"
 *
 * Unknown command
 *   Response — "ERROR"
 *
 * Design notes
 * ------------
 * - Filenames may not contain spaces (space is the field delimiter).
 *   This is the key limitation of v1; a future version could use quoting
 *   or a binary length-prefix for the filename too.
 * - PUT uses a length-prefix for content so binary files (images, archives)
 *   are transmitted correctly without escaping.
 * - There is no authentication or access control in v1.
 */
class FileStorageServer {
public:
    FileStorageServer(int domain, ServerType serverType = SYNC, std::string storagePath = "./");
    ~FileStorageServer();

    int server_bind(int domain, int port);
    int server_listen(int backlog);
    int start(int maxEvents = 10);

private:
    int serverSocket_;
    ServerType serverType_;
    std::string storagePath_;
    std::vector<int> clientSockets_;
    std::vector<struct pollfd> pollFds_;

    std::string readRequest(int clientSocket);
    std::string handleRequest(const std::string& request);
    std::string getFile(const std::string& request);
    std::string putFile(const std::string& request);
    std::string deleteFile(const std::string& request);
    std::string resolvePath(const std::string& filePath) const;

    int sendAll(int clientSocket, const std::string& response);
    void closeClient(int clientSocket);

    int runSync();
    int runSelect();
    int runPoll();
    int runEpoll(int maxEvents);
};

#endif // FILE_STORAGE_SERVER_H
