include(FetchContent)
set(gtest_force_shared_crt on)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG HEAD
)

FetchContent_MakeAvailable(googletest)

add_subdirectory(stl)
add_subdirectory(shm)
add_subdirectory(publish_subscribe)