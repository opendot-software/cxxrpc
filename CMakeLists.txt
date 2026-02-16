cmake_minimum_required(VERSION 3.10)
project(cxxrpc)

# add_executable(
#     main
#     ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc
# )

add_executable(
    threadpinning
    ${CMAKE_CURRENT_SOURCE_DIR}/src/threadpinning.cc
)

target_link_libraries(
    threadpinning
    hwloc
)