add_subdirectory(shm)
add_subdirectory(sock)
add_subdirectory(container)

add_library(ipcpp STATIC types.cpp sock/error.cpp shm/address_space.cpp shm/error.cpp)