add_subdirectory(shm)

add_library(ipcpp STATIC types.cpp)
target_link_libraries(ipcpp PUBLIC shm)