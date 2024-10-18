#include <ipcpp/publisher/publisher.h>
#include <ipcpp/publisher/notifier.h>
#include <ipcpp/types.h>

#include <iostream>

struct Notification {
  ipcpp::notification::NotificationType notification_type = ipcpp::notification::NotificationType::UNINITIALIZED;
  void* location = nullptr;
  std::size_t size = 0;
  uint64_t timestamp = 0;
};


int main() {
  auto expected_publisher = ipcpp::publisher::Publisher<ipcpp::publisher::DomainSocketNotifier, Notification>::create("/tmp/ipcpp.sock");

  if (expected_publisher.has_value()) {
    auto& publisher = expected_publisher.value();
    publisher.enable_registration();

    while (true) {
      std::string msg;
      std::getline(std::cin, msg);

      if (msg == "exit") {
        break;
      }

      Notification notification;
      notification.size = msg.size();

      publisher.publish<std::string>(std::move(msg));
    }
  }

  return 0;
}
