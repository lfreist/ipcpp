# --- publish/subscribe ------------------------------------------------------------------------------------------------
add_executable(shm_broadcaster shm_broadcaster.cpp)
target_link_libraries(shm_broadcaster PUBLIC shm stl spdlog::spdlog)

add_executable(shm_broadcast_client shm_broadcast_client.cpp)
target_link_libraries(shm_broadcast_client PUBLIC shm stl spdlog::spdlog)
# ======================================================================================================================

# --- shm notifier -----------------------------------------------------------------------------------------------------
add_executable(shm_polling_notifier shm_polling_notifier.cpp)
target_link_libraries(shm_polling_notifier PUBLIC shm stl spdlog::spdlog)
target_compile_definitions(shm_polling_notifier PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF)


add_executable(shm_polling_observer shm_polling_observer.cpp)
target_link_libraries(shm_polling_observer PUBLIC shm stl spdlog::spdlog)
target_compile_definitions(shm_polling_observer PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF)

# ======================================================================================================================

add_executable(mem_latency mem_latency.cpp)