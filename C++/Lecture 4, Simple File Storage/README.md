# Lecture 4 — Simple File Storage Server

**Status: 🔧 In Progress**

**Goal:** Build a real application on top of the `TCPServer` class from Lecture 3 — a file storage server with a hand-rolled GET/STORE/DELETE protocol — to understand *why* standard protocols like HTTP and FTP exist.

---

## What You Will Learn

- How to layer an **application protocol** on top of raw TCP
- Parsing structured text commands from a byte stream
- Why protocol design decisions (delimiters, quoting, status codes) matter
- The practical limitations that drove the creation of HTTP/FTP

---

## Architecture

`FileStorageServer` composes `TCPServer` rather than inheriting it, giving it full control over when to accept connections and how to dispatch requests.

```
FileStorageServer
    ├── TCPServer server_          (from Lecture 3)
    ├── std::string storagePath_   (directory where files live)
    ├── getFile(request)           parse GET, read file, return response
    ├── putFile(request)           parse STORE, write file to disk
    └── deleteFile(request)        parse DELETE, remove file from disk
```

Constructor signature:
```cpp
FileStorageServer(int domain, ServerType serverType = SYNC, std::string storagePath = "./");
```

The `ServerType` parameter is passed through to `TCPServer`, so the file server can run in sync, select, poll, or epoll mode.

---

## Wire Protocol (Version 1)

All commands are plain text sent over the TCP connection.

### GET

```
GET "[FILENAME]"
```

Response:
```
[STATUS_CODE] [FILE_CONTENT]
```

| Status Code | Meaning |
|-------------|---------|
| `0` | File found — content follows |
| `1` | File not found |

### STORE

```
STORE "[FILENAME]" [FILE_CONTENT]
```

No response defined in v1.

### DELETE

```
DELETE "[FILENAME]"
```

No response defined in v1.

**Why quote the filename?** Filenames can contain spaces. Without a delimiter, `GET my file.txt` is ambiguous — is it `my file.txt` or `my` with leftover `file.txt`? Quoting makes the boundary unambiguous.

---

## Why Not Just Use HTTP?

This lecture intentionally avoids HTTP to expose the problems it solves:

| Problem | This Protocol | HTTP |
|---------|--------------|------|
| Metadata (content-type, size) | None | Headers |
| Error details | Single int code | Status line + body |
| Framing (where does response end?) | Not specified | `Content-Length` / chunked encoding |
| Authentication | None | `Authorization` header |
| Caching | None | `ETag`, `Cache-Control` |

Once you feel the friction of solving these yourself, HTTP's complexity becomes justified.

---

## Files

| File | Purpose |
|------|---------|
| `FileStorageServer.h` | Class declaration + protocol design |
| `README` | Original protocol spec (plain text) |

**Still needed:**
- `FileStorageServer.cpp` — implement `getFile`, `putFile`, `deleteFile`
- `main.cpp` — wire up and run the server
- `Makefile` — build targets

---

## Exercises

1. Implement `FileStorageServer.cpp` — start with `getFile()` using `std::ifstream`.
2. Define a response for STORE and DELETE (success/failure status code).
3. Add a `LIST` command that returns all filenames in `storagePath_`.
4. Handle the edge case where `FILE_CONTENT` itself contains a quote character.
5. Rewrite the parser to be length-prefixed instead of quote-delimited — which is more robust?
6. Replace the custom protocol with HTTP/1.1 using only `<sys/socket.h>` — compare the complexity.

---

## What's Next

Possible future lectures:
- **Lecture 5** — HTTP/1.1 from scratch (parsing real requests/responses)
- **Lecture 6** — Async I/O frameworks: Boost.Asio or libuv
- **Lecture 7** — TLS with OpenSSL
