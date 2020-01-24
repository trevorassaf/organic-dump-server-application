#include "Server.h"

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>

#include "ClientHandler.h"
#include "NetworkUtilities.h"
#include "TlsConnection.h"
#include "TlsServer.h"
#include "TlsServerFactory.h"
#include "UndifferentiatedClientHandler.h"

namespace {
using network::TlsConnection;
using network::TlsServer;
using network::TlsServerFactory;
using organicdump_proto::ClientType;
using organicdump_proto::MessageType;

constexpr size_t kTimeoutSecs = 0;
constexpr size_t kTimeoutUsecs = 0;
} // namespace

namespace organicdump
{

bool Server::Create(
  int32_t port,
  std::string cert_file,
  std::string key_file,
  std::string ca_file,
  Server *out_server)
{
  TlsServer tls_server;
  TlsServerFactory server_factory;
  if (!server_factory.Create(
        cert_file,
        key_file,
        ca_file,
        port,
        network::WaitPolicy::NON_BLOCKING,
        &tls_server))
  {
    LOG(ERROR) << "Failed to create server factory";
    return false;
  }

  *out_server = Server{std::move(tls_server)};
  return true;
}

Server::Server(Server &&other)
{
  StealResources(&other);
}

Server &Server::operator=(Server &&other)
{
  if (this != &other)
  {
    StealResources(&other);
  }
  return *this;
}

Server::Server() {}

Server::Server(TlsServer tls_server)
  : tls_server_{std::move(tls_server)},
    clients_{},
    handlers_{}
{
    InitHandlers();
}

Server::~Server() {}

bool Server::Run()
{
  LOG(INFO) << "Starting organic dump server...";

  while (true)
  {
     struct timeval timeout;
     timeout.tv_sec = kTimeoutSecs;
     timeout.tv_usec = kTimeoutUsecs;

     fd_set read_fds;
     int max_fd = 0;

     for (auto &entry : clients_)
     {
       if (entry.first > max_fd)
       {
         max_fd = entry.first;
       }

       FD_SET(entry.first, &read_fds);
     }

     LOG(INFO) << "Entering select()...";

     int result = select(
         max_fd + 1,
         &read_fds,
         nullptr,
         nullptr,
         &timeout);

     switch (result)
     {
       case -1:
         LOG(ERROR) << "Failed during select(): " << strerror(errno);
         KickAllClients();
         return false;

      case 0:
         LOG(INFO) << "Select timeout expired: " << kTimeoutSecs << "s, "
                    << kTimeoutUsecs << "u";
         break;

      default:
         if (!ProcessReadableSockets(&read_fds, max_fd))
         {
           LOG(ERROR) << "Failed to process readable sockets";
           KickAllClients();
           return false;
         }

         LOG(INFO) << "Processed all readable sockets successfully";
         break;
     }
  }

  return true;
}

void Server::InitHandlers()
{
  assert(handlers_.empty());

  handlers_[ClientType::UNDIFFERENTIATED] =
      std::make_unique<UndifferentiatedClientHandler>();
  /*
  handlers_.emplace_back(
      organicdump_proto::ClientType::CONTROL,
      std::make_unique<ControlHandler>());
  handlers_.emplace_back(
      organicdump_proto::ClientType::RPI,
      std::make_unique<RpiHandler>());
      */
}

void Server::KickAllClients()
{
  LOG(ERROR) << "Kicking all clients and removing handlers";
  clients_.clear();
  handlers_.clear();
}

bool Server::ProcessReadableSockets(fd_set *readable_fds, int max_fd)
{
  assert(readable_fds);

  // First, check whether it's a new connection
  if (FD_ISSET(tls_server_.GetFd().Get(), readable_fds)) {
    TlsConnection cxn;
    if (!tls_server_.Accept(&cxn)) {
        LOG(ERROR) << "Failed to accept new connection";
    }
    else
    {
        LOG(INFO) << "Accepted new connection. Creating undifferented protobuf client";
        assert(clients_.count(cxn.GetFd().Get()) == 0);
        clients_.at(cxn.GetFd().Get()) = ProtobufClient{std::move(cxn)};
    }
  }

  // Finally, process read events for existing clients
  for (size_t fd = 0; fd <= max_fd; ++fd) {
    if (FD_ISSET(fd, readable_fds)) {
      LOG(INFO) << "Socket fd " << fd << " is readable";

      if (clients_.count(fd) == 0) {
        LOG(INFO) << "Fd " << fd << " marked as readable but client not found. "
                  << "Assume it was kicked in previous operation.";
        continue;
      }

      LOG(INFO) << "Found client for socket fd " << fd;

      // Read and handle message
      ProtobufClient *client = &clients_.at(fd);
      OrganicDumpProtoMessage msg;
      bool cxn_closed = false;

      if (!client->Read(&msg, &cxn_closed)) {
        if (cxn_closed) {
          LOG(ERROR) << "Connection closed by peer";
        }
        else
        {
          LOG(ERROR) << "Failed to read protobuf message. Kicking connection.";
        }
        client = nullptr;
        clients_.erase(fd);
        continue;
      }

      LOG(INFO) << ToString(msg.type) << " protobuf message read successfully from "
                << ToString(client->GetType()) << " client";

      // Client is already differentiated. Pass to relevant handler
      assert(handlers_.count(client->GetType()) == 1);

      ClientHandler *handler = handlers_.at(client->GetType()).get();
      if (!handler->Handle(msg, client, &clients_)) {
        LOG(ERROR) << "Failed to handle protobuf message. Kicking client.";
        clients_.erase(fd);
      }

      LOG(INFO) << "Protobuf message handled successfully";
    }
  }

  return true;
}

void Server::StealResources(Server *other)
{
    assert(other);

    tls_server_ = std::move(other->tls_server_);
    clients_ = std::move(other->clients_);
    handlers_ = std::move(other->handlers_);
}

}; // namespace organicdump
