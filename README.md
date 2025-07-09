# Multithreaded Proxy Web Server

A high-performance, multithreaded HTTP proxy server for Windows, written in C. This proxy supports caching with LRU eviction and colorful, informative terminal logs. Designed for learning, testing, and real-world use!

---

## 🚀 Features
- **Multithreaded:** Handles multiple client requests concurrently.
- **HTTP/1.0 & 1.1 Support:** For GET requests.
- **Caching:** In-memory cache with LRU (Least Recently Used) eviction policy.
- **Colorful Logging:** Terminal logs show request flow, cache hits/misses, and errors.
- **Customizable:** Easy to modify and extend.

---

## 🖥️ Requirements
- Windows OS
- [MinGW](http://www.mingw.org/) or similar GCC toolchain
- [Pthreads for Windows](https://sourceware.org/pthreads-win32/)
- [WinSock2](https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-start-page) (included with Windows)
- `make` (for using the provided Makefile)

---

## ⚙️ Build Instructions

1. **Clone the repository:**
   ```sh
   git clone https://github.com/Xensen008/Multithreaded-Proxy-Web-Server
   cd Multithreaded-Proxy-Web-Server
   ```

2. **Build using Makefile:**
   ```sh
   make
   ```
   This will compile `proxy_server_with_cache.c` and dependencies.

   > **Note:** If you encounter issues with pthreads or winsock, ensure your MinGW installation includes these libraries.

---

## 🏃 Usage

1. **Run the proxy server:**
   ```sh
   ./proxy <PORT>
   ```
   Example:
   ```sh
   ./proxy 8080
   ```

2. **Configure your browser or HTTP client** to use `localhost:<PORT>` as the HTTP proxy.

3. **Send HTTP GET requests** through the proxy. Example using `curl`:
   ```sh
   curl -x http://localhost:9000 http://example.com/
   ```
    or 
    http://localhost:8080/https://www.cs.princeton.edu
---

## 📝 Example Output
```
========================================
||      DANGER PROXY SERVER 9000!     ||
========================================
[12:34:56] [PROXY]      Setting Proxy Server Port : 9000
[12:34:57] [PROXY]      Binding on port: 9000
[12:35:01] [CLIENT]     [CONNECT] Port: 54321, IP: 127.0.0.1
[12:35:01] [CLIENT]     Received request from client
[12:35:01] [CACHE MISS] http://example.com/
[12:35:01] [FLOW]       Forwarding request to remote server: example.com/
[12:35:02] [CACHE STORE]http://example.com/
[12:35:02] [FLOW]       Response sent to client and cached.
```

---

## 🗂️ Project Structure
- `proxy_server_with_cache.c` — Main proxy server source code
- `proxy_parse.c`, `proxy_parse.h` — HTTP request parsing utilities
- `Makefile` — For building the project

---

## 🧠 Notes
- **Cache:** The proxy caches responses in memory. If the cache is full, the least recently used items are evicted.
- **Logging:** All major events (connections, cache, errors) are logged with color and tags for easy debugging.
- **Supported Methods:** Only HTTP GET is supported.
- **Platform:** Windows only (uses WinSock2 and pthreads for Windows).

---

## 🤝 Contributing
Pull requests and suggestions are welcome! Feel free to fork and adapt for your own use.

---

