#include <cassert>
#include <cstdlib>
#include <memory>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <TlsServerFactory.h>
#include <TlsServer.h>
#include <TlsConnection.h>
#include <TlsUtilities.h>

namespace
{
DEFINE_int32(port, -1, "Port");
DEFINE_string(cert, "", "Certificate file");
DEFINE_string(key, "", "Private key file");
DEFINE_string(ca, "", "CA file");
DEFINE_string(message, "Default client message", "Message to send client");

using network::TlsServer;
using network::TlsServerFactory;
using network::TlsConnection;
} // anonymous namespace

int main(int argc, char **argv)
{
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();

  TlsServer server;
  TlsServerFactory server_factory;
  if (!server_factory.Create(
        FLAGS_cert,
        FLAGS_key,
        FLAGS_ca,
        FLAGS_port,
        &server))
  {
      LOG(ERROR) << "Failed to create server";
      return EXIT_FAILURE;
  }

  TlsConnection cxn;
  if (!server.Accept(&cxn))
  {
      LOG(ERROR) << "Failed to accept TlsServer connection";
      return EXIT_FAILURE;
  }

  uint8_t read_buffer[256];
  std::string client_message;
  if (!ReadTlsMessage(
        &cxn,
        read_buffer,
        sizeof(read_buffer),
        &client_message))
  {
      LOG(ERROR) << "Failed to read TLS message from client";
      return EXIT_FAILURE;
  }

  LOG(INFO) << "Message from TLS client: " << client_message;

  if (!SendTlsMessage(&cxn, FLAGS_message))
  {
      LOG(ERROR) << "Failed to send message to TLS client: " << FLAGS_message;
      return EXIT_FAILURE;
  }

  LOG(INFO) << "Sent message to TLS client";
  return EXIT_SUCCESS;
}
