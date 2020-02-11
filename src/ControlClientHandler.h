#ifndef ORGANICDUMP_SERVER_CONTROLCLIENTHANDLER_H
#define ORGANICDUMP_SERVER_CONTROLCLIENTHANDLER_H

#include <memory>
#include <unordered_map>


#include "ClientHandler.h"
#include "DbManager.h"
#include "ProtobufClient.h"
#include "OrganicDumpProtoMessage.h"

namespace organicdump
{

class ControlClientHandler : public ClientHandler
{
public:
  static bool Create(ControlClientHandler *out_handler);

public:
  ControlClientHandler();
  ControlClientHandler(DbManager db);
  virtual ~ControlClientHandler() {}
  ControlClientHandler(ControlClientHandler &&other);
  ControlClientHandler &operator=(ControlClientHandler &&other);
  bool Handle(
      const OrganicDumpProtoMessage &msg,
      ProtobufClient *client,
      std::unordered_map<int, ProtobufClient> *clients) override;

private:
  void CloseResources();
  void StealResources(ControlClientHandler *other);
  bool RegisterClient(const organicdump_proto::RegisterClient &msg);
  bool RegisterSoilMoistureSensor(const organicdump_proto::RegisterSoilMoistureSensor &msg);
  bool UpdatePeripheralOwnership(const organicdump_proto::UpdatePeripheralOwnership &msg);

private:
  ControlClientHandler(const ControlClientHandler &other);
  ControlClientHandler &operator=(const ControlClientHandler &other);

private:
  bool is_initialized_;
  DbManager db_;
};

} // namespace organicdump

#endif // ORGANICDUMP_SERVER_CONTROLCLIENTHANDLER_H
