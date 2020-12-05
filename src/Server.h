#ifndef ORGANICDUMP_SERVER_SERVER_H
#define ORGANICDUMP_SERVER_SERVER_H

#include <cstdint>
#include <memory>
#include <string>

#include "ClientHandler.h"
#include "ProtobufClient.h"
#include "TlsServer.h"

namespace organicdump
{

class Server
{
public:
  static bool Create(
      int32_t port,
      std::string cert_file,
      std::string key_file,
      std::string ca_file,
      Server *out_server);

public:
  Server();
  Server(
      network::TlsServer tls_server,
      std::unordered_map<organicdump_proto::ClientType,
                         std::unique_ptr<ClientHandler>> handlers);
  Server(Server &&other);
  Server &operator=(Server &&other);
  ~Server();
  bool Run();

private:
  void KickAllClients();
  bool ProcessReadableSockets(fd_set *readable_fds, int max_fd);
  void StealResources(Server *other);

private:
  Server(const Server &other) = delete;
  Server &operator=(const Server &other) = delete;

private:
  network::TlsServer tls_server_;
  std::unordered_map<int, ProtobufClient> fd_to_client_map_;
  std::unordered_map<organicdump_proto::ClientType,
                     std::unique_ptr<ClientHandler>> handlers_;
};

}; // namespace organicdump

#endif // ORGANICDUMP_SERVER_SERVER_H
