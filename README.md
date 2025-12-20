# ipcpp: Interprocess Communication Framework with Dynamic Memory Management

> **This repo is not stable and more a playground than production ready software (for now)!**

**ipcpp** is a multi-platform (Linux and Windows) interprocess communication (IPC) library written in C++23.
It simplifies data sharing between processes through event-driven and publish/subscribe patterns, with seamless shared
memory management.

ipcpp main focuses are
- low latency: shared memory IPC works at < 100ns (from publishing to accessing data)
- multi-platform API: When using the default public API, you can use one code base for Windows and Linux (macOS and Android are planned)
- dynamically sized data structures: ipcpp/stl provides a set of standard-C++-like containers that allow dynamically
  sized data typed in your shared data (the total size of the shared memory is still pre-allocated at runtime and cannot
  be resized)
