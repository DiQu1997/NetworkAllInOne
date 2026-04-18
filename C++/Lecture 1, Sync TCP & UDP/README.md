# Lecture 1 — Synchronous TCP & UDP

**Goal:** Understand the fundamental POSIX socket API by building the simplest possible client/server pairs using blocking, synchronous I/O.

---

## What You Will Learn

- How the OS exposes network communication through **file descriptors**
- The lifecycle of a TCP connection (server side vs client side)
- The difference between **TCP** (connection-oriented) and **UDP** (connectionless)
- How to use `bind()`, `listen()`, `accept()`, `connect()`, `send()`/`recv()`, `sendto()`/`recvfrom()`
- Why "blocking" means the calling thread stalls until the operation completes

---

## Key Concepts

### TCP — Connection-Oriented

TCP guarantees ordered, reliable delivery. Both sides must agree to open a connection before any data flows.

**Server lifecycle:**
```
socket()  →  bind()  →  listen()  →  accept()  →  recv()/send()  →  close()
```

**Client lifecycle:**
```
socket()  →  connect()  →  send()/recv()  →  close()
```

### UDP — Connectionless

UDP sends self-contained datagrams with no handshake. Faster but no delivery guarantee.

**Server:** `socket()` → `bind()` → `recvfrom()` / `sendto()` → `close()`  
**Client:** `socket()` → `sendto()` / `recvfrom()` → `close()`  

No `listen()` or `accept()` — UDP has no concept of a connection.

### Special Addresses (IPv4)

| Constant | Value | Meaning |
|----------|-------|---------|
| `INADDR_ANY` | `0.0.0.0` | Bind to all available interfaces |
| `INADDR_LOOPBACK` | `127.0.0.1` | Loopback — same machine only |
| `INADDR_BROADCAST` | `255.255.255.255` | Broadcast to all hosts on subnet |

### Why "Synchronous" is a Problem

With a blocking `accept()` / `recv()`, the server can only handle **one client at a time**. A second client connecting will queue up (up to the `listen()` backlog limit) but cannot be served until the current client disconnects. This is the limitation Lecture 2 solves.

---

## Files

| File | Purpose |
|------|---------|
| `TCP/SyncTcpServer.cpp` | Blocking TCP server — one client at a time |
| `TCP/SyncTcpClient.cpp` | Blocking TCP client |
| `UDP/SyncUdpServer.cpp` | UDP server using `recvfrom`/`sendto` |
| `UDP/SyncUdpClient.cpp` | UDP client using `sendto`/`recvfrom` |

---

## Build & Run

```bash
# Using Makefile (from this directory)
make

# TCP: run server in one terminal, client in another
./tcp_server
./tcp_client

# UDP: same pattern
./udp_server
./udp_client
```

CMake alternative:
```bash
cd CMakeBuild && cmake .. && make
```

This lecture also includes Bazel (`BUILD`), Meson (`meson.build`), and Premake5 (`premake5.lua`) build files as a reference for build system comparison.

---

## Exercises

1. Modify the TCP server to echo back whatever the client sends (echo server).
2. Change the server address from `INADDR_ANY` to `INADDR_LOOPBACK` — what changes?
3. Send a large file over UDP. What happens to packets? Why?
4. Try connecting two TCP clients simultaneously — observe that the second one blocks.

---

## What's Next

**Lecture 2** solves the single-client limitation by introducing I/O multiplexing with `select()`, `poll()`, and `epoll()`, allowing one thread to serve many clients concurrently.
