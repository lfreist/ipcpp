add_executable(benchmark_event shm_notify_benchmark.cpp)
target_link_libraries(benchmark_event PRIVATE shm topic spdlog::spdlog)

add_executable(benchmark_pub_sub shm_pub_sub_benchmark.cpp)
target_link_libraries(benchmark_pub_sub PRIVATE shm topic spdlog::spdlog)

add_executable(benchmark_rt_pub_sub shm_rt_pub_sub_benchmark.cpp)
target_link_libraries(benchmark_rt_pub_sub PRIVATE shm topic spdlog::spdlog)


add_executable(benchmark_optional optional_benchmark.cpp)
