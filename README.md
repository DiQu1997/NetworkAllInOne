# NetworkAllInOne — Network Programming Tutorial

A hands-on, step-by-step C++ network programming course built around POSIX sockets. Each lecture introduces a new concept on top of the previous one, progressing from raw blocking sockets through multiplexed async I/O, object-oriented abstraction, and finally a real-world protocol implementation.

---

## Learning Path

| # | Lecture | Core Concept | Status |
|---|---------|-------------|--------|
| 1 | [Sync TCP & UDP](C++/Lecture%201%2C%20Sync%20TCP%20%26%20UDP/README.md) | Blocking sockets, TCP vs UDP | ✅ Complete |
| 2 | [Async I/O: select, poll, epoll](C++/Lecture%202%2C%20Async%20IO%20with%20selct%2C%20poll%2C%20epoll/README.md) | Multiplexing, non-blocking I/O | ✅ Complete |
| 3 | [OOP Refactor](C++/Lecture%203%2C%20OOP%20Refactor/README.md) | Encapsulation, strategy pattern | ✅ Complete |
| 4 | [Simple File Storage](C++/Lecture%204%2C%20Simple%20File%20Storage/README.md) | Custom protocol over TCP | 🔧 In Progress |
| 5 | *(planned)* HTTP/HTTPS basics | Standard application protocols | ⬜ Not Started |
| 6 | *(planned)* Async frameworks | Boost.Asio / libuv | ⬜ Not Started |

---

## Key Concepts Covered So Far

- **Socket API** — `socket()`, `bind()`, `listen()`, `accept()`, `connect()`, `send()`/`recv()`, `sendto()`/`recvfrom()`
- **TCP vs UDP** — connection-oriented reliable delivery vs connectionless datagrams
- **Blocking vs Non-blocking I/O** — synchronous one-client-at-a-time vs async multiplexing
- **I/O Multiplexing** — `select()` → `poll()` → `epoll()` with scalability tradeoffs
- **OOP Design** — wrapping raw C socket calls into reusable C++ classes
- **Protocol Design** — defining a custom request/response wire format over TCP
- **Build Systems** — CMake, Bazel, Meson, Premake5, and Makefile side-by-side

---

## Quick Start

Each lecture has its own `Makefile`. From any lecture directory:

```bash
make          # builds both server and client
make server   # builds only the server
make client   # builds only the client
make clean    # removes compiled binaries
```

For CMake builds:
```bash
cd CMakeBuild && cmake .. && make
```

---

## Progress Log

| Date | Commit | Milestone |
|------|--------|-----------|
| — | c30371e | Lecture 1: Basic TCP/UDP sync code, Makefile & CMake |
| — | ed8cc76 | Lecture 1: Add Bazel, Meson, Premake5 build support |
| — | eb8b8d4 | Lecture 2: TCP server with `select()` multiplexing |
| — | e9517ef | Lecture 2: TCP server with `poll()` |
| — | b9b08ee | Lecture 2: TCP server with `epoll()` |
| — | 02b47dc | Lecture 3: TCPServer class header |
| — | f7735e7 | Lecture 3: TCPServer class implementation |
| — | 64cf944 | Lecture 3: TCPClient class, OOP cleanup |
| — | be1999f | Lecture 4: FileStorageServer design + protocol spec |

---

## Repository Layout

```
NetworkAllInOne/
├── README.md                          ← you are here
└── C++/
    ├── Lecture 1, Sync TCP & UDP/
    │   ├── TCP/                       SyncTcpServer.cpp, SyncTcpClient.cpp
    │   ├── UDP/                       SyncUdpServer.cpp, SyncUdpClient.cpp
    │   └── CMakeBuild/
    ├── Lecture 2, Async IO with selct, poll, epoll/
    │   ├── select/                    TCPServer_select.cpp, TCPClient_select.cpp
    │   ├── poll/                      TCPServer_poll.cpp, TCPClient_poll.cpp
    │   └── epoll/                     TCPServer_epoll.cpp, TCPClient_epoll.cpp
    ├── Lecture 3, OOP Refactor/
    │   ├── TCPServer.h / .cpp
    │   ├── TCPClient.h
    │   └── main.cpp
    └── Lecture 4, Simple File Storage/
        ├── FileStorageServer.h
        └── README
```
