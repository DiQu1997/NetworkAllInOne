# Lecture 4 — Simple File Storage Server

**Status: ✅ Working (v1)**

**Goal:** Build a real application on top of the `TCPServer` class from Lecture 3 — a file storage server with a hand-rolled GET/PUT/DELETE protocol — to understand *why* standard protocols like HTTP and FTP exist.

---

## What You Will Learn

- How to layer an **application protocol** on top of raw TCP
- Why TCP being a **byte stream** (not a message stream) forces you to think about framing
- Why a naive `PUT <file_name> <contents>` design is broken for binary data
- Length-prefix vs delimiter-based framing, and which to pick when
- The practical limitations that drove the creation of HTTP/FTP

---

## Architecture

`FileStorageServer` composes `TCPServer` rather than inheriting it, giving it full control over when to accept connections and how to dispatch requests.

```
FileStorageServer
    ├── serverSocket_              listening fd (created in ctor)
    ├── ServerType serverType_     SYNC | ASYNC_SELECT | ASYNC_POLL | ASYNC_EPOLL
    ├── std::string storagePath_   directory where files live
    │
    ├── readRequest(fd)            recv loop with PUT-aware framing
    ├── handleRequest(req)         dispatch on "GET "/"PUT "/"DELETE "
    ├── getFile / putFile / deleteFile   parse + hit disk
    └── runSync / runSelect / runPoll / runEpoll   four event loops
```

The `ServerType` parameter chooses the I/O strategy at runtime — same protocol, four different concurrency models.

---

## How to Run

```bash
make                                              # builds ./file_storage_server
./file_storage_server                             # sync, port 8080, ./storage
./file_storage_server 9000 poll ./files           # custom port, mode, path
./file_storage_server 9000 epoll ./files          # Linux only
```

CLI args (all optional): `port` (default `8080`), `mode` (`sync`/`select`/`poll`/`epoll`, default `sync`), `storagePath` (default `./storage`, auto-created).

Smoke test with `nc`:
```bash
printf 'PUT hello.txt 13 Hello, World!' | nc localhost 8080   # → OK
printf 'GET hello.txt'                  | nc localhost 8080   # → Hello, World!
printf 'DELETE hello.txt'               | nc localhost 8080   # → OK
```

`printf` (not `echo`) matters for `PUT` — `echo` adds a newline that would break the byte count.

---

## Wire Protocol (Version 1)

All commands are plain ASCII over TCP. The connection is closed by the server after the response — one request per connection, no pipelining.

| Command | Wire format | Response |
|---------|------------|----------|
| `GET` | `GET <filepath>` | raw file bytes, or `NOT_FOUND` |
| `PUT` | `PUT <filepath> <length> <content>` | `OK` or `ERROR` |
| `DELETE` | `DELETE <filepath>` | `OK` or `ERROR` |

`<length>` is a decimal byte count; `<content>` is exactly `<length>` bytes following the space.

The full byte-level layout is documented in [FileStorageServer.h](FileStorageServer.h) and the parsing logic is in [FileStorageServer.cpp](FileStorageServer.cpp).

---

## Why not just `PUT <file_name> <contents>`?

This is the most important design question in the whole lecture. The naive design seems obvious — why complicate it with a length field?

The answer is: **TCP doesn't know what a "message" is.** TCP is a byte stream. The kernel makes no promise that one `send()` produces one `recv()`, or vice versa. A 1MB write might arrive as 700 `recv` chunks; a thousand small writes might coalesce into one. The application is responsible for finding the boundary between one logical request and the next.

That responsibility is called **framing**, and a protocol that doesn't solve it is broken.

### Attempt 1 — `PUT <file_name> <contents>` (no terminator)

```
"PUT notes.txt Hello, World!"
```

**Problem: when does the server stop reading?**

The server calls `recv()` and gets some bytes. If the file is 5 MB, those bytes arrive over hundreds of `recv()` calls. After each one, the server has to decide: *was that the last chunk, or is more coming?* With no length and no terminator, there is no way to tell. The server would have to wait for the connection to close to know the request is complete — which means **every PUT requires closing the connection**, which means no keep-alive, which means a new TCP handshake (3 round trips) per file. Unworkable.

### Attempt 2 — `PUT <file_name> <contents>\n` (newline terminator)

```
"PUT notes.txt Hello, World!\n"
```

**Problem: the contents may contain a newline.**

Text files routinely have newlines. Now the server thinks the request ended halfway through the file, treats the rest as a new command (`World!`), and replies `ERROR` for an unknown command — corrupting the upload silently.

### Attempt 3 — pick a magic terminator like `\0\0END\0\0`

**Problem: arbitrary binary content.**

A `.zip`, `.png`, or `.tar` file contains every possible byte sequence. Whatever sentinel you pick, some file somewhere contains it. You'd need to **escape** the sentinel inside the payload, then unescape on read — doubling the parsing complexity and growing the payload.

### Attempt 4 — length-prefix (what we actually use)

```
"PUT notes.txt 13 Hello, World!"
              └─ length tells the server exactly how many bytes follow
```

**Why it works:**
- The server reads `PUT <filepath> <length> ` until it has parsed the length field.
- Then it reads exactly `<length>` more bytes — no scanning, no escaping, no ambiguity.
- The content is treated as opaque bytes. Newlines, nulls, binary garbage — all fine.
- The connection can stay open afterward (keep-alive) because the boundary is unambiguous.

This is exactly the approach HTTP took in 1996 with the `Content-Length` header. Same problem, same solution.

### Why `GET` and `DELETE` get away without a length

They have **no payload**. The whole request is a short ASCII command — `GET notes.txt` is maybe 20 bytes — that almost always arrives in a single `recv()`. The server does one `recv`, treats the result as the full request, and moves on. This is fragile in theory (a hostile client could send one byte at a time), but adequate for v1. A v2 would frame these too, perhaps with `\r\n` like HTTP.

### The general lesson

Application protocols over TCP must answer **"where does one message end?"** before doing anything else. Two answers dominate the industry:

| Approach | Examples | Tradeoff |
|----------|----------|----------|
| Length-prefix | HTTP `Content-Length`, gRPC, MQTT, most binary protocols | Fast, binary-safe, requires knowing length upfront |
| Delimiter-based | HTTP headers (`\r\n\r\n`), SMTP (`.\r\n`), JSON Lines | Streamable, text-friendly, requires escaping for binary |

HTTP famously uses **both**: `\r\n\r\n` to delimit headers, then `Content-Length` (or chunked encoding) to frame the body. Each is the right tool for its job.

---

## Why Not Just Use HTTP?

This lecture intentionally avoids HTTP to expose the problems it solves:

| Problem | This Protocol | HTTP |
|---------|--------------|------|
| Framing (when does the message end?) | Length-prefix for PUT only | `Content-Length` / chunked encoding |
| Metadata (content-type, mtime, etc.) | None | Headers |
| Error details | `OK` / `ERROR` / `NOT_FOUND` strings | Status code + reason phrase + body |
| Connection reuse | Closes after every request | Keep-alive |
| Authentication | None | `Authorization` header, cookies |
| Caching | None | `ETag`, `If-Modified-Since`, `Cache-Control` |
| Path traversal protection | None — `GET ../etc/passwd` works | Path normalization in well-written servers |

Once you feel the friction of solving these yourself, HTTP's complexity becomes justified.

---

## Files

| File | Purpose |
|------|---------|
| `FileStorageServer.h` | Class declaration + protocol spec block |
| `FileStorageServer.cpp` | Implementation of all four event loops + protocol parsing |
| `main.cpp` | CLI entry point — parses argv, wires up the server, calls `start()` |
| `Makefile` | Builds `file_storage_server` |
| `README` | Plain-text protocol notes (legacy — superseded by this README) |

---

## Known Sharp Edges

- **No spaces in filenames** — space is the field delimiter for PUT.
- **No path sanitization** — `GET ../../etc/passwd` would happily try to read it. A real server would reject any filepath that escapes `storagePath_`.
- **No keep-alive** — the server closes after every response. Fine for a tutorial; would need request framing for `GET`/`DELETE` to enable.
- **`epoll` mode is Linux only** — guarded with `#ifdef __linux__`.

---

## Exercises

1. Add a `LIST` command that returns all filenames in `storagePath_`.
2. Add path-traversal protection: reject any filepath that resolves outside `storagePath_`.
3. Implement keep-alive — frame `GET` and `DELETE` with a length too, so the server can keep reading on the same connection.
4. Replace the length-prefix with HTTP/1.1 framing (`\r\n\r\n` headers + `Content-Length`). Compare the parsing complexity.
5. Add basic authentication: prefix every request with `AUTH <token>` and reject mismatches.
6. Write a stress test: 1000 concurrent clients PUT/GET/DELETE in a loop. Compare throughput across `sync`/`select`/`poll`/`epoll` modes.

---

## What's Next

Possible future lectures:
- **Lecture 5** — HTTP/1.1 from scratch (parsing real requests/responses)
- **Lecture 6** — Async I/O frameworks: Boost.Asio or libuv
- **Lecture 7** — TLS with OpenSSL
