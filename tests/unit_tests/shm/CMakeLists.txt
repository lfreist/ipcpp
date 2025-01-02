add_executable(mapped_memory_test unix/mapped_memory_test.cpp)
target_link_libraries(mapped_memory_test PRIVATE shm topic spdlog::spdlog)