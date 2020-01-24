#ifndef ORGANICDUMP_SERVER_UNDIFFERENTIATEDCLIENTHANDLER_H
#define ORGANICDUMP_SERVER_UNDIFFERENTIATEDCLIENTHANDLER_H

#include <memory>
#include <unordered_map>

#include "ClientHandler.h"
#include "ProtobufClient.h"
#include "OrganicDumpProtoMessage.h"

namespace organicdump
{

class UndifferentiatedClientHandler : public ClientHandler
{
public:
  virtual ~UndifferentiatedClientHandler();
  bool Handle(
      const OrganicDumpProtoMessage &msg,
      ProtobufClient *client,
      std::unordered_map<int, ProtobufClient> *clients);
};

} // namespace organicdump

#endif // ORGANICDUMP_SERVER_UNDIFFERENTIATEDCLIENTHANDLER_H
