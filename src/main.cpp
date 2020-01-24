#include <cassert>
#include <cstdlib>
#include <memory>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <NetworkUtilities.h>
#include <TlsServerFactory.h>
#include <TlsServer.h>
#include <TlsConnection.h>
#include <TlsUtilities.h>

#include <test.pb.h>

#include "src/CliConfig.h"

namespace
{
using network::TlsServer;
using network::TlsServerFactory;
using network::TlsConnection;
using network::ProtobufMessageHeader;
using organicdump::CliConfig;

void InitLibraries(const char *app_name)
{
  google::InitGoogleLogging(app_name);

  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
  ERR_load_BIO_strings();
}

} // anonymous namespace

int main(int argc, char **argv)
{
  CliConfig config;
  if (!CliConfig::Parse(argc, argv, &config))
  {
      LOG(ERROR) << "Failed to parse CLI flags";
      return EXIT_FAILURE;
  }

  InitLibraries(argv[0]);

  LOG(INFO) << "Port: " << config.GetPort();
  LOG(INFO) << "Cert: " << config.GetCertFile();
  LOG(INFO) << "Key: " << config.GetKeyFile();
  LOG(INFO) << "Ca: " << config.GetCaFile();

  TlsServer server;
  TlsServerFactory server_factory;
  if (!server_factory.Create(
        config.GetCertFile(),
        config.GetKeyFile(),
        config.GetCaFile(),
        config.GetPort(),
        network::WaitPolicy::NON_BLOCKING,
        &server))
  {
      LOG(ERROR) << "Failed to create server";
      return EXIT_FAILURE;
  }

  LOG(INFO) << "After server_factory.Create()";

  TlsConnection cxn;
  if (!server.Accept(&cxn))
  {
      LOG(ERROR) << "Failed to accept TlsServer connection";
      return EXIT_FAILURE;
  }

  LOG(INFO) << "After server.Accept()";

  ProtobufMessageHeader header;
  if (!ReadTlsProtobufMessageHeader(&cxn, &header))
  {
      LOG(ERROR) << "Failed to read TLS protobuf header";
      return EXIT_FAILURE;
  }

  test_message::BasicStringMsg msg;
  auto buffer = std::make_unique<uint8_t[]>(header.size);

  if (!ReadTlsProtobufMessageBody(
          &cxn,
          buffer.get(),
          header.size,
          &msg)) 
  {
      LOG(ERROR) << "Failed to read TLS protobuf message body";
      return EXIT_FAILURE;
  }

  LOG(INFO) << "Message from client: " << msg.str();

  test_message::MessageType basic_str_type = test_message::MessageType::BASIC_STRING;
  test_message::BasicStringMsg basic_str;
  //basic_str.set_str(FLAGS_message);

  bool cxn_closed = false;
  if (!SendTlsProtobufMessage(
          &cxn,
          static_cast<uint8_t>(basic_str_type),
          &basic_str,
          &cxn_closed))
  {
      LOG(ERROR) << "Failed to send TLS protobuf message";
      return EXIT_FAILURE;
  }

  LOG(INFO) << "Sent message to TLS client";
  return EXIT_SUCCESS;
}
