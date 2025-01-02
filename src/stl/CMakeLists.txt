add_library(stl vector.cpp)
target_link_libraries(stl PRIVATE spdlog::spdlog)