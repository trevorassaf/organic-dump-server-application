#include "cli_config.h"

#include <fstream>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

namespace
{
constexpr int BAD_PORT = -1;

bool FailBadPort(const char *param, int32_t port)
{
    if (port == BAD_PORT)
    {
        LOG(ERROR) << "--" << param << " must be set";
        return false;
    }
    return true;
}

bool CheckFileExists(const char *param, const std::string &file_name)
{
    std::ifstream file{file_name};
    if (!file.is_open())
    {
        LOG(ERROR) << "--" << param << " must specify valid file: " << file_name;
        return false;
    }
    return true;
}

DEFINE_int32(port, BAD_PORT, "Port");
DEFINE_string(cert, "", "Certificate file");
DEFINE_string(key, "", "Private key file");
DEFINE_string(ca, "", "CA file");

DEFINE_validator(port, FailBadPort);
DEFINE_validator(cert, CheckFileExists);
DEFINE_validator(key, CheckFileExists);
DEFINE_validator(ca, CheckFileExists);
} // namespace

namespace server
{

bool CliConfig::Parse(int argc, char **argv, CliConfig *out_config)
{
  // Force glog to write to stderr
  FLAGS_logtostderr = 1;
  google::ParseCommandLineFlags(&argc, &argv, false);
  *out_config = CliConfig{
      FLAGS_port,
      FLAGS_cert,
      FLAGS_key,
      FLAGS_ca};
  return true; 
}

CliConfig::CliConfig() {}

CliConfig::CliConfig(
    int32_t port,
    std::string cert_file,
    std::string key_file,
    std::string ca_file)
  : port_{port},
    cert_file_{std::move(cert_file)},
    key_file_{std::move(key_file)},
    ca_file_{std::move(ca_file)}
{}

int32_t CliConfig::GetPort() const
{
    return port_;
}

const std::string& CliConfig::GetCertFile() const
{
    return cert_file_;
}

const std::string& CliConfig::GetKeyFile() const
{
    return key_file_;
}

const std::string& CliConfig::GetCaFile() const
{
    return ca_file_;
}

}; // namespace server

