# carry: Interprocess Communication Framework with Dynamic Memory Management

**carry** is a multi-platform (Linux and Windows) interprocess communication (IPC) framework written in C++23.
It simplifies data sharing between processes through event-driven and publish/subscribe patterns, with seamless shared
memory management.

carry main focuses are
- low latency: shared memory IPC works at < 2µs (from publishing to accessing data!)
- multi-platform API: When using the default public API, you can use one code base for Windows and Linux (macOS and Android are planned)
- extensibility: You can implement your own notifier/observer patterns and use them with carry's public API
- dynamically sized data structures: carry/stl provides a set of standard-C++-like containers that allow dynamically
  sized data typed in your shared data (the total size of the shared memory is still pre-allocated at runtime and cannot
  be resized)

## Key Features

- **Event Notification**: Lightweight event-driven communication using Unix domain sockets (Linux) or shared memory (Linux and Windows).
- **Publish/Subscribe**:
    - Shared memory-based communication for real-time data sharing.
    - Automatic resource management using publishers and subscribers - no manual allocations/deallocations needed.
- **Shared Memory Containers**:
    - **`carry::vector`**: A `std::vector`-adopted container for shared memory with dynamic allocations
    - Supports dynamic resizing and efficient cross-process access.

---

## Getting Started

### Example: Publish/Subscribe Using Shared Memory

The following example demonstrates a broadcaster publishing messages and a client subscribing to them.

#### 1. **Define the Shared Message Structure**

```cpp  
#include <carry/container/vector.h>

struct Message {
  std::int64_t timestamp;
  carry::vector<char> data;
  
  Message() : timestamp(carry::utils::timestamp()) {}
  Message(Message&& other) noexcept : timestamp(carry::utils::timestamp()), data(std::move(other.data)) {}
};
```  

#### 2. **Publisher (shared memory notifications)**

```cpp  
#include <carry/event/shm_atomic_notifier.h>
#include <carry/publish_subscribe/publisher.h>
#include <carry/carry.h>

#include <iostream>

#include "message.h"

int main(int argc, char** argv) {
  spdlog::set_level(spdlog::level::debug);
  if (!carry::initialize_dynamic_buffer(4096 * 4096)) {
    std::cerr << "Failed to initialize buffer" << std::endl;
    return 1;
  }

  carry::publish_subscribe::Publisher<Message> publisher("my_id");
  if (std::error_code error = publisher.initialize()) {
    return 1;
  }
  while (true) {
    std::cout << "Enter message: ";
    std::string line;
    std::getline(std::cin, line);
    Message msg;
    msg.data = carry::vector<char>(line.begin(), line.end());
    publisher.publish(std::move(msg));
    if (line == "exit") {
      break;
    }
  }
}
```  

#### 3. **Client (Subscriber)**

```cpp  
#include <carry/publish_subscribe/subscriber.h>
#include <carry/event/shm_atomic_observer.h>
#include <carry/carry.h>

#include <iostream>

#include "message.h"

std::error_code receive_callback(carry::publish_subscribe::Subscriber<Message, carry::event::ShmAtomicObserver>::data_access_type& data) {
  auto value = data.acquire();
  const std::int64_t ts = carry::utils::timestamp();
  const std::string_view message(value->data.data(), value->data.size());
  if (message == "exit") {
    return {1, std::system_category()};
  }
  std::cout  << "latency: " << ts - value->timestamp << " - " << message << std::endl;
  return {};
}

int main() {
  spdlog::set_level(spdlog::level::debug);
  carry::initialize_dynamic_buffer();

  carry::publish_subscribe::Subscriber<Message> subscriber("my_id");
  if (const std::error_code error = subscriber.initialize(); error) {
    std::cerr << "Failed to initialize subscriber" << std::endl;
    return 1;
  }
  if (const auto error = subscriber.subscribe(); error) {
    std::cerr << "Failed to subscribe" << std::endl;
    return 1;
  }
  while (true) {
    if (const auto error = subscriber.receive(receive_callback, 0ms); error) {
      std::cout << "received exit message" << std::endl;
      break;
    }
  }
};
```  

---

### Compile and Run

carry is built using CMake:

```bash
mkdir build && cd build
cmake ..
make
```

---