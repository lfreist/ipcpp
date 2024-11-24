# ipcpp: Interprocess Communication Framework with Dynamic Memory Management

**ipcpp** is a modern C++23 framework for fast and reliable interprocess communication (IPC). It simplifies data sharing between processes through event-driven and publish/subscribe patterns, with seamless shared memory management.

## Key Features
- **Event Notification**: Lightweight event-driven communication using Unix domain sockets.
- **Publish/Subscribe**:
    - Shared memory-based communication for real-time data sharing.
    - Automatic resource management using broadcast publishers and subscribers.
- **Shared Memory Containers**:
    - **`ipcpp::vector`**: A `std::vector`-adopted container for shared memory with dynamic allocations
    - Supports dynamic resizing and efficient cross-process access.

---

## Getting Started

### Example: Publish/Subscribe Using Shared Memory

The following example demonstrates a broadcaster publishing messages and a client subscribing to them.

#### 1. **Define the Shared Message Structure**

```cpp  
#include <ipcpp/datatypes/vector.h>

struct Message {
  std::int64_t timestamp;
  ipcpp::vector<char> data;
};
```  

#### 2. **Broadcaster (Publisher)**

```cpp  
#include <ipcpp/publish_subscribe/broadcast_publisher.h>
#include <ipcpp/utils/io.h>
#include <spdlog/spdlog.h>

#include "message.h"

int main() {
  auto expected_broadcaster = ipcpp::publish_subscribe::Broadcaster<Message>::create(
      "broadcaster", 10, 2048, 4096);  // Shared memory buffer configuration

  if (expected_broadcaster.has_value()) {
    auto& broadcaster = expected_broadcaster.value();
    while (true) {
      Message message{.timestamp = ipcpp::utils::timestamp(), .data = ipcpp::vector<char>()};
      std::cout << "Enter message: ";
      while (true) {
        char c;
        std::cin.get(c);
        if (c == '\n') {
          break;
        }
        message.data.push_back(c);
      }
      broadcaster.publish(message);
      spdlog::info("Published: {}", std::string_view(message.data.data(), message.data.size()));
    }
  } else {
    std::cerr << "Failed to create broadcaster." << std::endl;
  }
}
```  

#### 3. **Client (Subscriber)**

```cpp  
#include <ipcpp/publish_subscribe/broadcast_subscriber.h>
#include <spdlog/spdlog.h>

#include "message.h"

int main() {
  auto expected_subscriber = ipcpp::publish_subscribe::BroadcastSubscriber<Message>::create("broadcaster");

  if (!expected_subscriber.has_value()) {
    std::cerr << "Error creating subscriber: " << expected_subscriber.error() << std::endl;
    return 1;
  }

  auto& subscriber = expected_subscriber.value();
  spdlog::info("Listening for messages...");
  subscriber.on_receive([](ipcpp::publish_subscribe::BroadcastSubscriber<Message>::Data<ipcpp::AccessMode::READ>& data) {
    std::cout << "Received: " << std::string_view(data.data().data.data(), data.data().data.size()) << std::endl;
  });

  return 0;
}
```  

---

### Compile and Run

ipcpp is built using CMake:

```bash
mkdir build && cd build
cmake ..
make
```

---

## Roadmap
- **Allocator Traits**: Enhanced flexibility for `ipcpp` allocators.
- **Language Bindings**: Support for Python and Rust.
- **Additional Patterns**: Request/Response, Pipes, and more.

---