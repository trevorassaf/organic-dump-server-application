cmake_minimum_required(VERSION 3.1)

add_executable(test_crypto_server test_crypto_server.cpp)
target_link_libraries(test_crypto_server gflags::gflags)
target_link_libraries(test_crypto_server glog::glog)
target_link_libraries(test_crypto_server ssl crypto)
