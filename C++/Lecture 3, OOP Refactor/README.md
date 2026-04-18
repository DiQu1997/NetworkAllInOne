# Lecture 3 — OOP Refactor

**Goal:** Eliminate duplicated socket boilerplate from Lectures 1–2 by encapsulating TCP server and client logic into reusable C++ classes with a clean interface.

---

## What You Will Learn

- How to wrap C-style POSIX socket calls inside C++ classes
- The **strategy pattern** — selecting an algorithm (sync, select, poll, epoll) at runtime via an enum
- How to design a header-only interface (`TCPServer.h`, `TCPClient.h`) that hides implementation details
- Common OOP pitfalls with socket resources (lifetime, RAII, rule-of-three/five)

---

## Design Overview

### `TCPServer`

```
TCPServer(domain)
    ├── server_bind(port)         bind socket to port
    ├── server_listen(backlog)    start listening
    ├── server_accept()           blocking accept (sync mode)
    ├── sever_select()            multiplexed loop via select()
    ├── server_poll()             multiplexed loop via poll()
    ├── server_epoll(max_events)  multiplexed loop via epoll()
    └── server_send(fd, data)     send to a specific client fd
```

The `ServerType` enum lets callers declare their intent:

```cpp
enum ServerType { SYNC, ASYNC_SELECT, ASYNC_POLL, ASYNC_EPOLL };
TCPServer server(AF_INET);
server.server_bind(8080);
server.server_listen(5);
server.server_epoll(64);   // or server_accept(), server_poll(), etc.
```

### `TCPClient`

```
TCPClient
    ├── client_connect(ip, port)  connect to server
    ├── client_send(data)         send data
    ├── client_receive()          receive data
    └── client_close()            close connection
```

---

## Files

| File | Purpose |
|------|---------|
| `TCPServer.h` | Class declaration, `ServerType` enum, helper `set_socket_blocking()` |
| `TCPServer.cpp` | Implementation of all server methods |
| `TCPClient.h` | TCPClient class declaration |
| `main.cpp` | Minimal usage example (sync mode, port 8080) |

---

## Key Implementation Notes

- **`set_socket_blocking(fd, false)`** — calls `fcntl(fd, F_SETFL, O_NONBLOCK)` to toggle blocking mode, shared by select/poll/epoll paths.
- **`clientSockets_`** vector — tracks all accepted client fds for the multiplexed modes.
- **`serverSocket_`** member — holds the listening fd; opened in constructor, closed in destructor (RAII).
- There is a known typo: `sever_select()` should be `server_select()` — worth fixing as an exercise.

---

## Build & Run

```bash
make
./server   # runs main.cpp which uses SYNC mode on port 8080
```

---

## Exercises

1. Fix the typo: rename `sever_select()` to `server_select()` in both `.h` and `.cpp`.
2. Add a destructor to `TCPServer` that calls `close(serverSocket_)` and `close()` on each element of `clientSockets_`.
3. Implement the `Rule of Five` — add copy constructor, copy assignment, move constructor, move assignment.
4. Add a `ServerType` parameter to the constructor and call the right `server_*()` method from a single `run()` method.
5. Write a unit test that instantiates `TCPServer`, binds to an ephemeral port, and verifies `listen()` succeeds.

---

## What's Next

**Lecture 4** uses `TCPServer` as a base to build a real application: a file storage server with a custom GET/STORE/DELETE protocol.
