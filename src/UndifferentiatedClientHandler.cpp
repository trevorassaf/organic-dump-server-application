#include "UndifferentiatedClientHandler.h"

#include <memory>
#include <unordered_map>

#include "ProtobufClient.h"
#include "OrganicDumpProtoMessage.h"

namespace
{
using organicdump_proto::ClientType;
using organicdump_proto::Hello;
using organicdump_proto::MessageType;
} // namespace

namespace organicdump
{

UndifferentiatedClientHandler::~UndifferentiatedClientHandler() {}

bool UndifferentiatedClientHandler::Handle(
    const OrganicDumpProtoMessage &msg,
    ProtobufClient *client,
    std::unordered_map<int, ProtobufClient> *clients)
{
  assert(client);
  assert(clients);
  assert(!client->IsDifferentiated());

  if (msg.type != MessageType::HELLO) {
    LOG(ERROR) << "Expected HELLO message in undifferentiated client. Received message "
               << "w/type " << ToString(msg.type);
    return false;
  }

  const Hello &hello = msg.hello;

  LOG(INFO) << "Received Hello from client w/type: " << ToString(hello.type())
            << " and ID: " << hello.client_id();

  client->Differentiate(hello.type(), hello.client_id());
  return true;
}

}; // namespace organicdump
