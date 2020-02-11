#include "ControlClientHandler.h"

#include <memory>
#include <unordered_map>

#include <mysqlx/xdevapi.h>

#include "ProtobufClient.h"
#include "OrganicDumpProtoMessage.h"
#include "SqlUtils.h"

namespace
{
using organicdump_proto::ClientType;
using organicdump_proto::MessageType;
using organicdump_proto::RegisterClient;

using ClientMap = std::unordered_map<int, organicdump::ProtobufClient>;

} // namespace

namespace organicdump
{

bool ControlClientHandler::Create(ControlClientHandler *out_handler)
{
  assert(out_handler);

  DbManager db;
  if (!DbManager::Create(&db))
  {
    LOG(ERROR) << "Failed to create DbManager";
    return false;
  }

  *out_handler = ControlClientHandler{std::move(db)};
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
    ClientMap *clients)
{
  assert(client);
  assert(clients);
  assert(client->IsDifferentiated());
  assert(client->GetType() == ClientType::CONTROL);

  switch (msg.type) {
    case MessageType::REGISTER_CLIENT:
      return RegisterClient(msg.register_client);
    case MessageType::REGISTER_SOIL_MOISTURE_SENSOR:
      return RegisterSoilMoistureSensor(msg.register_soil_moisture_sensor);
    case MessageType::UPDATE_PERIPHERAL_OWNERSHIP:
      return UpdatePeripheralOwnership(msg.update_peripheral_ownership);
    default:
      LOG(ERROR) << "Received unexpected message from Control Client: " << MessageType_Name(msg.type);
      return false;
  }
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

bool ControlClientHandler::RegisterClient(const organicdump_proto::RegisterClient &msg)
{
  // Throw error if client already exists with this name

  // Insert new record
  return false;
}

bool ControlClientHandler::RegisterSoilMoistureSensor(
    const organicdump_proto::RegisterSoilMoistureSensor &msg)
{
  // Validate soil moisture records
  if (!db_.ContainsRpi(msg.meta().rpi_id())) {
    LOG(ERROR) << "RPI does not exist. ID: " << msg.meta().rpi_id();
    return false;
  }

  if (db_.ContainsPeripheral(msg.meta().name())) {
    LOG(ERROR) << "Peripheral already exists with name: " << msg.meta().name();
    return false;
  }

  // Insert records for soil moisture sensor
  size_t id;

  if (!db_.InsertSoilMoistureSensor(
          msg.meta().name(),
          msg.floor(),
          msg.ceil(),
          &id)) {
    LOG(ERROR) << "Failed to insert soil moisture sensor";
  }

  return true;
}

bool ControlClientHandler::UpdatePeripheralOwnership(
    const organicdump_proto::UpdatePeripheralOwnership &msg)
{
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

  return true;
}

} // namespace organicdump
