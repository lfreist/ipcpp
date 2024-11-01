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