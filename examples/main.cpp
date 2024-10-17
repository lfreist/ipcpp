#include <concepts>
#include <expected>
#include <string>
#include <utility>

// Define the concept to check for create method
template <typename T, typename E, typename... Args>
concept HasCreate = requires(Args&&... args) {
  { T::create(std::forward<Args>(args)...)} -> std::same_as<std::expected<T, E>>;
};

// Templated class
template <typename N>
class DomainSocketNotificationHandler {
 public:
  // Static member function create
  static std::expected<DomainSocketNotificationHandler<N>, int> create(std::string socket_path) {
    // Implementation here
    return DomainSocketNotificationHandler<N>();
  }
};

// Check if DomainSocketNotificationHandler meets the HasCreate concept


int main() {
  static_assert(HasCreate<DomainSocketNotificationHandler<int>, int, std::string>); // Replace int with the actual type you want to test

}