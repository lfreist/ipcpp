add_executable(main main.cpp)

add_executable(publisher publisher.cpp)
add_executable(subscriber subscriber.cpp)

add_executable(shm_publisher shm_publisher.cpp)
target_link_libraries(shm_publisher PUBLIC ipcpp shm sock)
add_executable(shm_subscriber shm_subscriber.cpp)
target_link_libraries(shm_subscriber PUBLIC ipcpp shm sock)

add_executable(shared_alloc shared_allocator_example.cpp)
target_link_libraries(shared_alloc PUBLIC shm)

add_executable(domain_socket_notifier domain_socket_notifier.cpp)
target_link_libraries(domain_socket_notifier PUBLIC shm sock spdlog::spdlog)

add_executable(domain_socket_observer domain_socket_observer.cpp)
target_link_libraries(domain_socket_observer PUBLIC shm sock spdlog::spdlog)

add_executable(shm_polling_notifier shm_polling_notifier.cpp)
target_link_libraries(shm_polling_notifier PUBLIC shm spdlog::spdlog)

add_executable(shm_polling_observer shm_polling_observer.cpp)
target_link_libraries(shm_polling_observer PUBLIC shm spdlog::spdlog)

add_executable(shm_broadcaster shm_broadcaster.cpp)
target_link_libraries(shm_broadcaster PUBLIC shm sock spdlog::spdlog)

add_executable(shm_broadcast_client shm_broadcast_client.cpp)
target_link_libraries(shm_broadcast_client PUBLIC shm sock spdlog::spdlog)

add_executable(shm_list_allocator shm_list_allocator.cpp)
target_link_libraries(shm_list_allocator PUBLIC shm spdlog::spdlog)