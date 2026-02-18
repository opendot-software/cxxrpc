cmake_minimum_required(VERSION 3.10)
project(cxxrpc)

# ---- main ---- #
add_executable(
    main
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc
)

# ---- swc [opendot::cxxrpc::pipeline] ---- #
# add_executable(
#     pipeline
#     ${CMAKE_CURRENT_SOURCE_DIR}/software-components/pseudo-code/software-pipeline/pseudo-pipeline.cc
# )

# ---- swc [opendot::cxxrpc::thread_manager] ---- #
add_executable(
    threadpool
    ${CMAKE_CURRENT_SOURCE_DIR}/software-components/pseudo-code/thread-pool/pseudo-thread-pool.cc
)

target_link_libraries(
    threadpool
    hwloc
)