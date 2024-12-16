// #define LOGGING_LEVEL LOG_LEVEL_OFF
#include <ipcpp/utils/logging.h>

int main() {
  ipcpp::logging::info("Hello, World! {}", 1);
  ipcpp::logging::critical("Hello, World!");
}