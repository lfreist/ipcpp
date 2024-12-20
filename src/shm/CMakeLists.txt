add_library(shm
        shared_memory_file.cpp unix/shared_memory_file.cpp windows/shared_memory_file.cpp
        mapped_memory.cpp unix/mapped_memory.cpp windows/mapped_memory.cpp)
target_link_libraries(shm PRIVATE spdlog::spdlog)