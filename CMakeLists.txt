cmake_minimum_required(VERSION 3.9)
project(ipcpp CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED 23)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Winterference-size INTERFERENCE_SIZE_WARNING)

if (INTERFERENCE_SIZE_WARNING)
    add_compile_options(-Wno-interference-size)
endif (INTERFERENCE_SIZE_WARNING)


set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif ()

include(FetchContent)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.0
)

FetchContent_MakeAvailable(spdlog)

include_directories(${PROJECT_SOURCE_DIR}/include/)
add_subdirectory(src)

IF (${MAIN_PROJECT})
    # ___ Executables __________________________________________________________________________________________________
    add_subdirectory(examples)

    # ___ Benchmarks ___________________________________________________________________________________________________
    add_subdirectory(benchmarks)

    # ___ Tests ________________________________________________________________________________________________________
    include(CTest)
    add_subdirectory(tests)
endif ()