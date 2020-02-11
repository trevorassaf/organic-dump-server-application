#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "CliConfig.h"
#include "Server.h"

namespace
{
using organicdump::CliConfig;
using organicdump::Server;

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
  std::cout << "bozkurtus -- main() -- before CliConfig::Parse()" << std::endl;

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

  Server server;
  if (!Server::Create(
        config.GetPort(),
        config.GetCertFile(),
        config.GetKeyFile(),
        config.GetCaFile(),
        &server)) {
    LOG(ERROR) << "Failed to initialize organic dump server";
    return EXIT_FAILURE;
  }

  if (!server.Run()) {
    LOG(ERROR) << "Failed to run organic dump server";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
