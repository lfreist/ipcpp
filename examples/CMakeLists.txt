# --- publish/subscribe ------------------------------------------------------------------------------------------------
add_executable(shm_broadcaster shm_broadcaster.cpp)
target_link_libraries(shm_broadcaster PUBLIC shm stl topic spdlog::spdlog)

add_executable(shm_broadcast_client shm_broadcast_client.cpp)
target_link_libraries(shm_broadcast_client PUBLIC shm stl topic spdlog::spdlog)

# ======================================================================================================================

add_executable(mem_latency mem_latency.cpp)

# === quick tests
add_executable(impl_tests main.cpp)
target_link_libraries(impl_tests PUBLIC shm stl topic spdlog::spdlog)