#ifndef ORGANICDUMP_SERVER_CLIENTHANDLER_H
#define ORGANICDUMP_SERVER_CLIENTHANDLER_H

#include <memory>
#include <unordered_map>

#include "ProtobufClient.h"
#include "OrganicDumpProtoMessage.h"

namespace organicdump
{

class ClientHandler
{
public:
  virtual ~ClientHandler() {}
  virtual bool Handle(
      const OrganicDumpProtoMessage &msg,
      ProtobufClient *client,
      std::unordered_map<int, ProtobufClient> *clients) = 0;
};

}; // namespace organicdump

#endif // ORGANICDUMP_SERVER_CLIENTHANDLER_H
