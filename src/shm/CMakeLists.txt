add_library(shm shared_memory_file.cpp mapped_memory.cpp)
target_link_libraries(shm PRIVATE spdlog::spdlog)