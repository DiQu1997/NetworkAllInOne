# Lecture 2 — Async I/O: select, poll, epoll

**Goal:** Serve multiple clients concurrently in a **single thread** by using I/O multiplexing — letting the OS notify you when a socket is ready, instead of blocking on one at a time.

---

## The Core Problem from Lecture 1

In Lecture 1, `accept()` and `recv()` are **blocking calls** — the thread sleeps until one specific socket has something to do. If you have 30 clients, you need 30 threads (or processes), which is expensive and complex.

I/O multiplexing solves this with a single call that watches many sockets at once and wakes you up only when at least one is ready:

```
while (true) {
    ready = select/poll/epoll(all_watched_fds)  // blocks here
    for each fd in ready:
        if fd == server_socket → accept new client, add to watch list
        else                   → read data from that client
}
```

This pattern — **the event loop** — is the backbone of nginx, Node.js, Redis, and most high-performance servers.

---

## `select()` — [select/TCPServer_select.cpp](select/TCPServer_select.cpp)

### How it works

`select()` watches file descriptors using a **bit array** called `fd_set`. Each bit position represents one fd number. You tell the kernel which bits (fds) you care about; it flips the bits of the ones that are ready.

```cpp
fd_set readfds;

// Every loop iteration — rebuild from scratch
FD_ZERO(&readfds);                    // clear all bits
FD_SET(master_socket, &readfds);      // bit for server socket
for (auto &sd : client_sockets) {
    if (sd > 0) FD_SET(sd, &readfds); // bit for each client
}

// Blocks until ≥1 fd is ready; modifies readfds in place
activity = select(max_sd + 1, &readfds, nullptr, nullptr, nullptr);

// After select returns — check each fd manually
if (FD_ISSET(master_socket, &readfds)) { /* new connection */ }
for (auto &sd : client_sockets) {
    if (FD_ISSET(sd, &readfds)) { /* data ready on this client */ }
}
```

The first argument `max_sd + 1` is critical: `select()` scans **every bit from 0 to max_sd**, so the range must cover all your fds.

### Why it has a 1024 fd limit

`fd_set` is a fixed-size C struct — typically 128 bytes, giving 1024 bits. If any fd number is ≥ 1024, `FD_SET` silently writes out of bounds (undefined behavior). This is a hard architectural limit, not a tunable parameter.

### The rebuild problem

Notice `FD_ZERO` and all the `FD_SET` calls happen **inside the loop**, before every `select()` call. That's because `select()` overwrites `readfds` with the result — it destroys your input. You always pay the cost of rebuilding the set even if nothing changed.

### Summary of limitations

| Problem | Details |
|---------|---------|
| Max 1024 fds | Fixed by `fd_set` struct size |
| Rebuilds every loop | `FD_ZERO` + N × `FD_SET` before every call |
| Scans 0 → max_fd | Even fds you don't own get scanned by the kernel |
| Fixed-size client array | `std::vector<int> client_sockets(30, 0)` — wastes slots |

---

## `poll()` — [poll/TCPServer_poll.cpp](poll/TCPServer_poll.cpp)

### How it works

`poll()` replaces `fd_set` with a **dynamic array of `struct pollfd`**. Each element explicitly names an fd and what events to watch. The kernel writes results into `revents` without destroying `events`.

```cpp
std::vector<struct pollfd> fds;

// Setup — done once per fd, not every loop
struct pollfd listener;
listener.fd     = listener_fd;
listener.events = POLLIN;   // interested in: data to read
fds.push_back(listener);

// The call
int ret = poll(fds.data(), fds.size(), -1);  // -1 = wait forever

// After poll returns — scan the array
for (int i = 0; i < fds.size(); i++) {
    if (fds[i].revents & POLLIN) {          // kernel wrote result here
        if (fds[i].fd == listener_fd) { /* new connection */ }
        else                           { /* client data */   }
    }
}

// Adding a new client — just push_back, no rebuild
struct pollfd client_fd;
client_fd.fd     = new_socket;
client_fd.events = POLLIN;
fds.push_back(client_fd);

// Removing a client — erase from vector
fds.erase(fds.begin() + i);
```

### `events` vs `revents` — the key design improvement

`select()` uses the same `fd_set` for input and output, destroying it each call. `poll()` separates them:
- `events` — what **you** want to watch (write once when adding the fd)
- `revents` — what the **kernel** found ready (written by `poll()`, read by you)

This means you only rebuild when a client connects or disconnects, not every loop iteration.

### Event flags

| Flag | Direction | Meaning |
|------|-----------|---------|
| `POLLIN` | events | There is data to read |
| `POLLOUT` | events | Socket buffer has room to write |
| `POLLERR` | revents | Error on the fd (kernel-set only) |
| `POLLHUP` | revents | Other side closed the connection |
| `POLLNVAL` | revents | fd is not open |

### Why poll is still O(n)

`poll()` is better than `select()`, but the kernel still has to walk the entire array to set `revents`. If you have 10,000 fds but only 1 fires, the kernel checked 9,999 others for nothing. This is the problem epoll solves.

### Summary of improvements over select

| Feature | select | poll |
|---------|--------|------|
| fd limit | 1024 (hard) | Unlimited |
| Input structure | Destroyed each call | Preserved (`revents` separate) |
| Adding fds | Rebuild whole set | `push_back` |
| Removing fds | Mark slot as 0 | `erase` from vector |
| Scan cost | O(max_fd) | O(n active fds) |

---

## `epoll()` — [epoll/TCPServer_epoll.cpp](epoll/TCPServer_epoll.cpp)

### How it works

`epoll` is a three-syscall API. The kernel maintains a **persistent interest list** for you — you register fds once and never rebuild it. `epoll_wait()` returns only the fds that are actually ready.

```cpp
// 1. Create the epoll instance (a kernel object, returned as an fd)
epoll_fd = epoll_create1(0);

// 2. Register the server socket — done ONCE
struct epoll_event ev;
ev.events  = EPOLLIN;
ev.data.fd = server_fd;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

// Event loop
struct epoll_event events[MAX_EVENTS];
while (true) {
    // 3. Wait — returns only ready events, not all watched fds
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

    for (int n = 0; n < nfds; ++n) {
        if (events[n].data.fd == server_fd) {
            // New connection — register the new client fd
            new_socket = accept(server_fd, ...);
            ev.events  = EPOLLIN;
            ev.data.fd = new_socket;
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev);  // ADD
        } else {
            // Data from client — read it
            ssize_t count = read(events[n].data.fd, buffer, sizeof(buffer));
            if (count == 0) {
                close(events[n].data.fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, nullptr);  // DEL
            } else {
                write(events[n].data.fd, buffer, count);
            }
        }
    }
}
```

### The three epoll_ctl operations

```cpp
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);   // start watching fd
epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);   // change what events to watch
epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr); // stop watching fd
```

Each is O(log n) — the kernel stores the interest list in a red-black tree.

### Why epoll_wait is O(ready), not O(n)

When a watched fd becomes ready, the kernel adds it to a separate **ready list** (a linked list). `epoll_wait()` just drains that list — it never looks at the fds that aren't ready. With 10,000 connected clients and 1 active, you process exactly 1 event.

### Level-triggered vs edge-triggered

`epoll` supports two notification modes, controlled by event flags:

**Level-triggered (default, no flag needed):**
- `epoll_wait` keeps returning the fd as ready as long as data remains unread.
- Easier to use — you can read partial data and catch the rest next loop.
- Same behavior as `select` and `poll`.

**Edge-triggered (`EPOLLET` flag):**
```cpp
ev.events = EPOLLIN | EPOLLET;
```
- `epoll_wait` fires **only when the state changes** (new data arrives), not while data sits unread.
- You **must** drain the entire buffer per event (loop `read()` until `EAGAIN`), or data will be silently stuck.
- More complex but avoids redundant wakeups under high load.

The code in [TCPServer_epoll.cpp](epoll/TCPServer_epoll.cpp) uses the default level-triggered mode.

### Summary of improvements over poll

| Feature | poll | epoll |
|---------|------|-------|
| Interest list location | User space (rebuilt) | Kernel (persistent) |
| `epoll_wait` return | All watched fds | Ready fds only |
| Event dispatch cost | O(n watched) | O(ready events) |
| fd limit | Unlimited | Unlimited (`ulimit -n`) |
| Platform | POSIX | Linux only |
| Trigger modes | Level only | Level or Edge |

---

## Side-by-Side: The Full Picture

```
select()                  poll()                    epoll()
──────────────────────    ──────────────────────    ──────────────────────
fd_set (bit array)        struct pollfd[]           kernel interest list
1024 fd limit             unlimited fds             unlimited fds
rebuilds every call       events persists           persistent, use _ctl
scans 0→max_fd            scans all n entries       returns ready only
O(max_fd)                 O(n)                      O(ready)
POSIX portable            POSIX portable            Linux only
```

### When to use which

| Situation | Recommendation |
|-----------|---------------|
| Portability matters (BSD, macOS, Windows) | `select` or `poll` |
| < 100 connections, simplicity preferred | `poll` |
| > 1000 connections, Linux server | `epoll` |
| Production Linux server | `epoll` (or a library wrapping it: libuv, Boost.Asio) |

---

## Other Important Details in the Code

### `SO_REUSEADDR` (select server, line 53)
```cpp
setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```
When a server closes, the TCP stack holds the port in `TIME_WAIT` state for ~60 seconds to catch delayed packets. Without `SO_REUSEADDR`, restarting the server immediately gives "Address already in use". This option bypasses that wait.

### `fcntl` non-blocking (client files)
```cpp
fcntl(fd, F_SETFL, O_NONBLOCK);
```
In non-blocking mode, `recv()` on an empty socket returns `-1` with `errno == EAGAIN` instead of sleeping. The clients in this lecture use this so they can use `select`/`poll` to wait for writability before sending, rather than blocking indefinitely.

---

## Build & Run

Each subdirectory has its own `Makefile`:

```bash
# Terminal 1 — start the server
cd select && make && ./server

# Terminal 2 — connect a client
./client

# Repeat with poll/ and epoll/ — client behavior is identical
```

---

## Exercises

1. Connect 5 clients simultaneously to the `select` server — verify all are served concurrently.
2. In the `poll` server, add a 5-second idle timeout: pass `5000` ms instead of `-1` to `poll()`, and print a message when it times out.
3. Change the `epoll` server to edge-triggered mode (`EPOLLET`). What breaks? Fix it by draining the buffer in a loop until `EAGAIN`.
4. Add a broadcast: when one client sends a message, forward it to every other connected client. Implement this in each of the three servers.
5. Measure: write a script that opens 500 simultaneous connections and sends data. Compare throughput between the three implementations.
