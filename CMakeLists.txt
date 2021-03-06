cmake_minimum_required(VERSION 3.1)

include_directories(
  .
  ../../organic-dump-network/repo
  ../../organic-dump-protocols/repo
  ../../../external/protobuf/repo/src
  ../../../external/boringssl/repo/include
  ../../mysql-cpp-prebuilts/repo/include
)

add_executable(test_crypto_server examples/test_crypto_server.cpp)
target_link_libraries(test_crypto_server gflags::gflags)
target_link_libraries(test_crypto_server glog::glog)
target_link_libraries(test_crypto_server ssl crypto)
target_link_libraries(test_crypto_server organic_dump_network)
target_link_libraries(test_crypto_server test_proto)
#target_link_libraries(test_crypto_server mysqlcppconn)

add_executable(organic_dump_server
  src/main.cpp
  src/CliConfig.cpp
  src/ControlClientHandler.cpp
  src/DbManager.cpp
  src/ProtobufClient.cpp
  src/Server.cpp
  src/UndifferentiatedClientHandler.cpp)

file(GLOB MYSQL_PREBUILT_LIBS "../../mysql-cpp-prebuilts/repo/lib/*.so*")
message("BOZKURTUS -- MYSQL LIBRARIES = ${MYSQL_PREBUILT_LIBS}")
target_link_libraries(organic_dump_server ${MYSQL_PREBUILT_LIBS})

target_link_libraries(organic_dump_server gflags::gflags)
target_link_libraries(organic_dump_server glog::glog)
target_link_libraries(organic_dump_server ssl crypto)
target_link_libraries(organic_dump_server organic_dump_network)
target_link_libraries(organic_dump_server organic_dump_proto)
