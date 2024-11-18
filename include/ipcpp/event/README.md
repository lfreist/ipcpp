# Events

> Events implement event of clients (observers) about an event. Observers may be actively polling for an event or
> are notified using e.g. domain sockets if an event is published.

## Notifications

Notifications are sent by a single *Notifier* to subscribed *Observers*. *Observers* can subscribe to a *Notifier* and
cancel their subscription (cancellation). *Notifiers* implement the [*Notifier_I*](notifier.h)-Interface while
*Observers* implement the [*Observer_I*](observer.h)-Interface.

### Unix Domain Socket Notifications

[DomainSocketNotifier](domain_socket_notifier.h) and [DomainSocketObserver](domain_socket_observer.h) implement these
interfaces and provide the functionality using Unix Domain Sockets.

A *DomainSocketNotifier* accepts subscriptions when the `DomainSocketNotifier::accept_subscriptions()` was called.
*DomainSocketObservers* can now subscribe themselves to the Observer and cancel their subscription on the fly.

#### Example Usage: Notifier

```c++
// domain_socket_notifier_main.cpp

#include <ipcpp/event/domain_socket_notifier.h>
#include <ipcpp/utils/utils.h>

struct Notification {
  int64_t timestamp;
};

int main(int argc, char** argv) {
        // retrieve a DomainSocketNotifier that accepts a maximum of 5 subscribers, broadcasting a Notification
        auto expected_notifier = ipcpp::event::DomainSocketNotifier<Notification, int>::create("/tmp/ipcpp.sock", 5);
        if (!expected_notifier.has_value()) {
                std::cerr << "Failed to create DomainSocketNotifier: " << expected_notifier.error() << std::endl;
        }

        auto& notifier = expected_notifier.value();
        
        // accept subscriptions
        notifier.accept_subscriptions();
        
        while (true) {
                std::cout << "Press 'enter' to send a event" << std::endl;
                std::string tmp;
                std::getline(std::cin, tmp);
                
                if (tmp == "exit") {
                        break;
                }
                
                // broadcast the current timestamp
                Notification event {
                        .timestamp = ipcpp::utils::timestamp()
                };
                notifier.notify_observers(event);
        }
}
```

#### Example Usage: Observer

```c++
// domain_socket_observer_main.cpp

#include <ipcpp/event/domain_socket_observer.h>
#include <ipcpp/utils/utils.h>

#include <chrono>

struct Notification {
        int64_t timestamp;
};

int main(int argc, char** argv) {
        using namespace std::chrono_literals;
        auto expected_observer = ipcpp::event::DomainSocketObserver<Notification, int>::create("/tmp/ipcpp.sock");
        if (!expected_observer.has_value()) {
                std::cerr << "Failed to create DomainSocketObserver: " << expected_observer.error() << std::endl;
                return 1;
        }

        auto& observer = expected_observer.value();
        
        auto subscription_result = observer.subscribe();
        if (subscription_result.has_value()) {
                std::cout << "Successfully subscribed to Notifier" << std::endl;
        } else {
                std::cerr << "Subscription failed: " << subscription_result.error() << std::endl;
        }
        
        while (true) {
                auto result = observer.receive([](Notification event) -> int64_t {
                        int64_t timestamp = ipcpp::utils::timestamp();
                        return timestamp - event.timestamp;
                });
                if (result.has_value()) {
                        std::cout << "Received event with latency: " << result.value() << std::endl;
                } else {
                        std::cout << "Error receiving message: " << result.error() << std::endl;
                        break;
                }
        }
}
```