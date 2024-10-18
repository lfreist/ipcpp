add_executable(main main.cpp)

add_executable(publisher publisher.cpp)
add_executable(subscriber subscriber.cpp)

add_executable(shm_publisher shm_publisher.cpp)
target_link_libraries(shm_publisher PUBLIC ipcpp)
add_executable(shm_subscriber shm_subscriber.cpp)
target_link_libraries(shm_subscriber PUBLIC ipcpp)