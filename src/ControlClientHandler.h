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
  bool RegisterRpi(
      const organicdump_proto::RegisterRpi &msg,
      ProtobufClient *client);
  bool RegisterSoilMoistureSensor(
      const organicdump_proto::RegisterSoilMoistureSensor &msg,
      ProtobufClient *client);
  bool UpdatePeripheralOwnership(
      const organicdump_proto::UpdatePeripheralOwnership &msg,
      ProtobufClient *client);

private:
  bool SendSuccessfulBasicResponse(size_t id, ProtobufClient *client);
  bool SendFailedBasicResponse(
      organicdump_proto::ErrorCode code,
      const std::string& message,
      ProtobufClient *client);

private:
  ControlClientHandler(const ControlClientHandler &other);
  ControlClientHandler &operator=(const ControlClientHandler &other);

private:
  bool is_initialized_;
  DbManager db_;
};

} // namespace organicdump

#endif // ORGANICDUMP_SERVER_CONTROLCLIENTHANDLER_H
