#ifndef SERVER_CLICONFIG_H
#define SERVER_CLICONFIG_H

#include <cstdint>

#include <gflags/gflags.h>
#include <glog/logging.h>

namespace server
{

class CliConfig
{
public:
  static bool Parse(int argc, char **argv, CliConfig *out_config);

public:
  CliConfig();
  CliConfig(
      int32_t port,
      std::string cert_file,
      std::string key_file,
      std::string ca_file);

  int32_t GetPort() const;
  const std::string& GetCertFile() const;
  const std::string& GetKeyFile() const;
  const std::string& GetCaFile() const;

private:
  int32_t port_;
  std::string cert_file_;
  std::string key_file_;
  std::string ca_file_;
};

}; // namespace server

#endif // SERVER_CLICONFIG_H
