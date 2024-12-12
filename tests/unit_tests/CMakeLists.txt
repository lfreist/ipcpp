include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP OFF
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(stl)