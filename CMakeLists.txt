cmake_minimum_required(VERSION 3.1)

include_directories(../../organic-dump-network/repo)

add_executable(test_crypto_server test_crypto_server.cpp)
target_link_libraries(test_crypto_server gflags::gflags)
target_link_libraries(test_crypto_server glog::glog)
target_link_libraries(test_crypto_server ssl crypto)
target_link_libraries(test_crypto_server organic_dump_network)
add_dependencies(test_crypto_server protoc)
