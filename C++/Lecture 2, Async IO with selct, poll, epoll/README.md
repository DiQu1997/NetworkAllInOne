# Lecture 2 — Async I/O: select, poll, epoll

**Goal:** Serve multiple clients concurrently in a **single thread** by using I/O multiplexing — letting the OS notify you when a socket is ready, instead of blocking on one at a time.

---

## What You Will Learn

- What "non-blocking" socket mode means and how to set it with `fcntl()`
- How `select()`, `poll()`, and `epoll()` differ in API and scalability
- The event loop pattern — the backbone of nearly every network server and framework
- Why `epoll()` is preferred for high-concurrency servers on Linux

---

## The Core Problem

In Lecture 1, `accept()` and `recv()` block — the thread sleeps until one specific socket is ready. With multiple clients you'd need multiple threads. Multiplexing lets a single call watch many sockets at once and return only the ready ones.

```
while (true) {
    ready_fds = select/poll/epoll(all_watched_fds)
    for each fd in ready_fds:
        if fd == server_socket → accept new client
        else                   → read data from existing client
}
```

---

## The Three Mechanisms

### `select()` — [select/](select/)

Uses a **bit array** (`fd_set`) to represent watched file descriptors.

```cpp
FD_ZERO(&read_fds);
FD_SET(server_fd, &read_fds);
select(max_fd + 1, &read_fds, NULL, NULL, NULL);
if (FD_ISSET(fd, &read_fds)) { /* fd is ready */ }
```

**Limitations:**
- Hard limit of **1024 file descriptors** (`FD_SETSIZE`)
- Must rebuild `fd_set` every loop iteration — O(max_fd) overhead
- Scans all fds up to `max_fd` even if only one is ready

### `poll()` — [poll/](poll/)

Uses a **dynamic array** of `struct pollfd` instead of a bit array.

```cpp
struct pollfd fds[MAX];
fds[0] = { server_fd, POLLIN, 0 };
poll(fds, nfds, -1);
if (fds[i].revents & POLLIN) { /* fd is ready */ }
```

**Improvements over select:**
- No 1024 fd limit (array can be any size)
- Cleaner separation of `events` (what to watch) vs `revents` (what fired)

**Still O(n):** must scan the entire array to find ready fds.

### `epoll()` — [epoll/](epoll/)

Linux-specific. Uses a **kernel-managed interest list**; returns only the ready events.

```cpp
int epfd = epoll_create1(0);
epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);   // register once
int n = epoll_wait(epfd, events, MAX_EVENTS, -1);  // blocks until ready
for (int i = 0; i < n; i++) { /* events[i] is already ready */ }
```

**Advantages:**
- **O(1) per event** — kernel only returns what's ready
- No fd limit beyond system ulimit
- Supports edge-triggered (`EPOLLET`) vs level-triggered modes
- Scales to 100k+ concurrent connections

---

## Comparison Table

| Feature | `select` | `poll` | `epoll` |
|---------|---------|--------|--------|
| Max fds | 1024 | Unlimited | Unlimited |
| Complexity | O(max_fd) | O(n) | O(ready) |
| Rebuild per loop | Yes (`fd_set`) | Yes (array) | No |
| Portability | POSIX (all) | POSIX (all) | Linux only |
| Scalability | Poor | Moderate | Excellent |

---

## Files

| File | Purpose |
|------|---------|
| `select/TCPServer_select.cpp` | Multi-client server using `select()` |
| `select/TCPClient_select.cpp` | Non-blocking client using `select()` |
| `poll/TCPServer_poll.cpp` | Multi-client server using `poll()` |
| `poll/TCPClient_poll.cpp` | Non-blocking client using `poll()` |
| `epoll/TCPServer_epoll.cpp` | Multi-client server using `epoll()` |
| `epoll/TCPClient_epoll.cpp` | Non-blocking client using `epoll()` |

---

## Build & Run

Each subdirectory has its own `Makefile`:

```bash
# select
cd select && make && ./server &  ./client

# poll
cd poll && make && ./server &  ./client

# epoll
cd epoll && make && ./server &  ./client
```

---

## Notable Details

- **`SO_REUSEADDR`** — set on the server socket so you can restart the server immediately without waiting for the `TIME_WAIT` state to expire.
- **`fcntl(fd, F_SETFL, O_NONBLOCK)`** — sets a socket to non-blocking; a `recv()` on an empty socket returns `-1` with `errno == EAGAIN` instead of blocking.
- **`EPOLLET` (edge-triggered)** — epoll fires only when the state *changes* (new data arrives), not as long as data is available. Requires draining the entire buffer per event.

---

## Exercises

1. Connect 5 clients simultaneously to the `select` server — verify all are served.
2. Swap the `select` server for `epoll` with identical client code — observe no difference from the client's perspective.
3. Add a load test: write a script that opens 500 connections to the `epoll` server.
4. Implement a broadcast: when one client sends a message, the server forwards it to all other connected clients.

---

## What's Next

**Lecture 3** wraps these three mechanisms inside C++ classes, eliminating repeated boilerplate and letting callers choose the I/O strategy at runtime.
