add_executable(main main.cpp)

# --- publish/subscribe ------------------------------------------------------------------------------------------------
add_executable(shm_broadcaster shm_broadcaster.cpp)
target_link_libraries(shm_broadcaster PUBLIC shm sock spdlog::spdlog_header_only)

add_executable(shm_broadcast_client shm_broadcast_client.cpp)
target_link_libraries(shm_broadcast_client PUBLIC shm spdlog::spdlog_header_only)
# ======================================================================================================================

# --- shm notifier -----------------------------------------------------------------------------------------------------
add_executable(shm_polling_notifier shm_polling_notifier.cpp)
target_link_libraries(shm_polling_notifier PUBLIC shm spdlog::spdlog_header_only)

add_executable(shm_polling_observer shm_polling_observer.cpp)
target_link_libraries(shm_polling_observer PUBLIC shm spdlog::spdlog_header_only)
# ======================================================================================================================
