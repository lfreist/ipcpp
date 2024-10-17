#include <ipcpp/subscriber/subscriber.h>
#include <ipcpp/subscriber/notification_handler.h>
#include <ipcpp/types.h>

#include <numeric>

struct Notification {
  bool exit = false;
  void* location = nullptr;
  std::size_t size = 0;
  uint64_t timestamp = 0;
};

int main() {
  using namespace ipcpp::subscriber;
  static_assert(ipcpp::concepts::HasCreate<DomainSocketNotificationHandler<Notification>, int, std::string>); // Replace int with the actual type you want to test

  auto expected_subscriber = Subscriber<DomainSocketNotificationHandler, Notification>::create("/tmp/ipcpp.sock");

  if (expected_subscriber.has_value()) {
    auto& subscriber = expected_subscriber.value();
    std::atomic_bool stop_flag(false);
    if (subscriber.register_to_publisher()) {
      while (true) {
        auto value = subscriber.receive_one(
            [](Notification notification) {
              std::cout << "Received " << notification.size << " bytes" << std::endl;
              return true;
            });
        if (!value.has_value()) {
          if (value.error() == -1) {
            break;
          }
        }
      }
    }
  }

  return 0;
}
