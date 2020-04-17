#include "ControlClientHandler.h"

#include <memory>
#include <unordered_map>

#include <mysqlx/xdevapi.h>

#include "ProtobufClient.h"
#include "OrganicDumpProtoMessage.h"
#include "SqlUtils.h"

#define UNUSED(x) (void)(x)

namespace
{
using organicdump_proto::ClientType;
using organicdump_proto::BasicResponse;
using organicdump_proto::ErrorCode;
using organicdump_proto::MessageType;
using organicdump_proto::RegisterRpi;

using ClientMap = std::unordered_map<int, organicdump::ProtobufClient>;

} // namespace

namespace organicdump
{

bool ControlClientHandler::Create(ControlClientHandler *out_handler)
{
  LOG(ERROR) << "bozkurtus -- ControlClientHander::Create() -- call";
  assert(out_handler);

  DbManager db;
  if (!DbManager::Create(&db))
  {
    LOG(ERROR) << "Failed to create DbManager";
    return false;
  }

  LOG(ERROR) << "bozkurtus -- ControlClientHander::Create() -- after DbManager::Create()";

  *out_handler = ControlClientHandler{std::move(db)};
  LOG(ERROR) << "bozkurtus -- ControlClientHander::Create() -- end";
  return true;
}

ControlClientHandler::ControlClientHandler() : is_initialized_{false} {}

ControlClientHandler::ControlClientHandler(DbManager db)
  : is_initialized_{true},
    db_{std::move(db)} {}

ControlClientHandler::ControlClientHandler(ControlClientHandler &&other)
{
  StealResources(&other);
}

ControlClientHandler &ControlClientHandler::operator=(ControlClientHandler &&other)
{
  if (this != &other)
  {
    CloseResources();
    StealResources(&other);
  }

  return *this;
}

bool ControlClientHandler::Handle(
    const OrganicDumpProtoMessage &msg,
    ProtobufClient *client,
    ClientMap *all_clients)
{
  LOG(ERROR) << "bozkurtus -- ControlClientHander::Handle() -- call";

  assert(client);
  assert(all_clients);
  assert(client->IsDifferentiated());
  assert(client->GetType() == ClientType::CONTROL);

  LOG(ERROR) << "bozkurtus -- ControlClientHander::Handle() -- before switch()";

  switch (msg.type) {
    case MessageType::REGISTER_RPI:
      return RegisterRpi(msg.register_rpi, client);
    case MessageType::REGISTER_SOIL_MOISTURE_SENSOR:
      return RegisterSoilMoistureSensor(msg.register_soil_moisture_sensor, client);
    case MessageType::UPDATE_PERIPHERAL_OWNERSHIP:
      return UpdatePeripheralOwnership(msg.update_peripheral_ownership, client);
    case MessageType::SEND_SOIL_MOISTURE_MEASUREMENT:
      return StoreSoilMoistureMeasurement(msg.send_soil_moisture_measurement, client);
    default:
      LOG(ERROR) << "Received unexpected message from Control Client: " << MessageType_Name(msg.type);
      return false;
  }
  LOG(ERROR) << "bozkurtus -- ControlClientHander::Handle() -- end";
}

void ControlClientHandler::CloseResources()
{
  is_initialized_ = false;
  db_ = DbManager{};
}

void ControlClientHandler::StealResources(ControlClientHandler *other)
{
  assert(other);
  is_initialized_ = other->is_initialized_;
  other->is_initialized_ = false;
  db_ = std::move(other->db_);
}

bool ControlClientHandler::RegisterRpi(
    const organicdump_proto::RegisterRpi &msg,
    ProtobufClient *client)
{
  UNUSED(client);

  LOG(ERROR) << "bozkurtus -- ControlClientHandler::RegisterRpi() -- call";

  if (db_.ContainsRpi(msg.name()))
  {
    LOG(ERROR) << "RPi already exists with name: " << msg.name();

    return SendFailedBasicResponse(
        ErrorCode::INVALID_PARAMETER,
        "RPi with that name already exists",
        client);
  }

  size_t id; 
  if (!db_.InsertRpi(
        msg.name(),
        msg.location(),
        &id))
  {
    LOG(ERROR) << "Failed to insert RPi record";

    SendFailedBasicResponse(
        ErrorCode::INTERNAL_SERVER_ERROR,
        "Failed to insert RPi record",
        client);

    return false;
  }

  LOG(INFO) << "Registered RPi with ID: " << id;

  if (!SendSuccessfulBasicResponse(id, client))
  {
    LOG(ERROR) << "Failed to send basic response with rpi id: " << id;
    return false;
  }

  LOG(ERROR) << "bozkurtus -- ControlClientHandler::RegisterRpi() -- end";
  return true;
}

bool ControlClientHandler::RegisterSoilMoistureSensor(
    const organicdump_proto::RegisterSoilMoistureSensor &msg,
    ProtobufClient *client)
{
  UNUSED(client);

  if (msg.meta().has_rpi_id() && !db_.ContainsRpi(msg.meta().rpi_id())) {
    LOG(ERROR) << "RPI does not exist. ID: " << msg.meta().rpi_id();
    return false;
  }

  if (db_.ContainsPeripheral(msg.meta().name())) {
    LOG(ERROR) << "Peripheral already exists with name: " << msg.meta().name();
    return false;
  }

  size_t id;
  if (!db_.InsertSoilMoistureSensor(
          msg.meta().name(),
          msg.floor(),
          msg.ceil(),
          &id)) {
    LOG(ERROR) << "Failed to insert soil moisture sensor";
    return false;
  }

  LOG(INFO) << "Registered soil moisture sensor with ID: " << id;

  if (!SendSuccessfulBasicResponse(id, client))
  {
    LOG(ERROR) << "Failed to send successful response to client.";
    return false;
  }

  return true;
}

bool ControlClientHandler::UpdatePeripheralOwnership(
    const organicdump_proto::UpdatePeripheralOwnership &msg,
    ProtobufClient *client)

{
  UNUSED(client);

  LOG(INFO) << "Updating peripheral ownership: "
            << "rpi_id=" << msg.rpi_id() << ", "
            << "peripheral_id=" << msg.peripheral_id();

  // Ensure RPI and peripheral both exist
  if (!db_.ContainsRpi(msg.rpi_id()))
  {
    LOG(ERROR) << "No RPI exists with id=" << msg.rpi_id();
    return false;
  }

  if (!db_.ContainsPeripheral(msg.peripheral_id()))
  {
    LOG(ERROR) << "No peripheral exists with id=" << msg.peripheral_id();
    return false;
  }

  // Remove current association record if it exists. If the request does not represent a
  // delete operation, add the new entry.
  db_.OrphanRpiOwnedPeripheral(msg.peripheral_id());

  // This request asks to delete the association, resulting in an orphaned peripheral
  if (msg.orphan_peripheral()) {
    return true;
  }

  if (!db_.AssignPeripheralToRpi(msg.rpi_id(), msg.peripheral_id()))
  {
    LOG(ERROR) << "Failed to reparent peripheral";
  }

  if (!SendSuccessfulBasicResponse(client))
  {
    LOG(ERROR) << "Failed to send basic response to client";
    return false;
  }

  return true;
}

bool ControlClientHandler::StoreSoilMoistureMeasurement(
      const organicdump_proto::SendSoilMoistureMeasurement &msg,
      ProtobufClient *client)
{
  assert(client);

  size_t measurement_id;

  if (!db_.InsertSoilMoistureMeasurement(
          msg.sensor_id(),
          msg.value(),
          &measurement_id))
  {
    LOG(ERROR) << "Failed to insert soil moisture measurement"
               << ". Id: " << msg.sensor_id()
               << ". Measurement: " << msg.value();
    return false;
  }

  if (!SendSuccessfulBasicResponse(measurement_id, client))
  {
    LOG(ERROR) << "Failed to send basic response to client";
    return false;
  }

  return true;
}

bool ControlClientHandler::SendSuccessfulBasicResponse(ProtobufClient *client)
{
  BasicResponse resp;
  resp.set_code(ErrorCode::OK);
  OrganicDumpProtoMessage msg{std::move(resp)};

  if (!client->Write(&msg))
  {
    LOG(ERROR) << "Failed to send successful basic response";
    return false;
  }

  return true;
}

bool ControlClientHandler::SendSuccessfulBasicResponse(
    size_t id,
    ProtobufClient *client)
{
  BasicResponse resp;
  resp.set_code(ErrorCode::OK);
  resp.set_id(id);
  OrganicDumpProtoMessage msg{std::move(resp)};

  if (!client->Write(&msg))
  {
    LOG(ERROR) << "Failed to send successful basic response";
    return false;
  }

  return true;
}

bool ControlClientHandler::SendFailedBasicResponse(
    organicdump_proto::ErrorCode code,
    const std::string& message,
    ProtobufClient *client)
{
  BasicResponse resp;
  resp.set_code(code);
  resp.set_message(message);
  OrganicDumpProtoMessage msg{std::move(resp)};

  if (!client->Write(&msg))
  {
    LOG(ERROR) << "Failed to send failed basic response";
    return false;
  }

  return true;
}

} // namespace organicdump
