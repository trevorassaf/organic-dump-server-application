#include "DbManager.h"

#include <cassert>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <utility>

#include <mysqlx/xdevapi.h>
#include <glog/logging.h>

namespace {
constexpr const char *DB_URL = "mysqlx://trevor@localhost";
constexpr const char *DB_NAME = "plantsandthings";

constexpr const char *RPIS_TABLE = "rpis";
constexpr const char *PERIPHERALS_TABLE = "peripherals";
constexpr const char *RPI_PERIPHERAL_EDGES_TABLE = "rpi_peripheral_edges";
constexpr const char *SOIL_MOISTURE_SENSORS_TABLE = "soil_moisture_sensors";
constexpr const char *SOIL_MOISTURE_MEASUREMENTS_TABLE = "soil_moisture_measurements";

std::string MakeTimestamp() {
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H-%M-%S");
  return oss.str();
}

bool ContainsRecordById(mysqlx::Schema *schema, const char *table_name, size_t id)
{
  assert(table_name);

  mysqlx::Table table = schema->getTable(table_name);

  size_t record_count =
      table.select()
          .where("id = :key")
          .bind(":key", id)
          .execute()
          .count();

  return record_count > 0;
}
} // namespace

namespace organicdump
{

bool DbManager::Create(DbManager *out_manager) {
  assert(out_manager);

  auto session = std::make_unique<mysqlx::Session>(DB_URL);
  bool check_db_existence = true;
  auto db = std::make_unique<mysqlx::Schema>(
      session->getSchema(DB_NAME, check_db_existence));

  if (!db->existsInDatabase()) {
    LOG(ERROR) << "Schema " << DB_NAME << " does not exist in db";
    return false;
  }

  *out_manager = DbManager{std::move(session), std::move(db)};
  return true;
}

DbManager::DbManager() : is_initialized_{false} {}

DbManager::DbManager(
    std::unique_ptr<mysqlx::Session> session,
    std::unique_ptr<mysqlx::Schema> db)
    : is_initialized_{true},
      session_{std::move(session)},
      db_{std::move(db)} {}

DbManager::~DbManager()
{
  CloseResources();
}

DbManager::DbManager(DbManager &&other)
{
  StealResources(&other);
}

DbManager &DbManager::operator=(DbManager &&other)
{
  if (this != &other)
  {
    CloseResources();
    StealResources(&other);
  }
  return *this;
}

bool DbManager::OrphanRpiOwnedPeripheral(size_t peripheral_id)
{
  mysqlx::Table edges_table = db_->getTable(RPI_PERIPHERAL_EDGES_TABLE);

  const mysqlx::Result result = edges_table
      .remove()
      .where("peripheral_id = :id")
      .bind(":id", peripheral_id)
      .execute();

  if (result.getAffectedItemsCount() == 0)
  {
    LOG(ERROR) << "Peripheral " << peripheral_id << " not owned by any rpi";
    return false;
  }

  return true;
}

bool DbManager::AssignPeripheralToRpi(size_t rpi_id, size_t peripheral_id)
{
  mysqlx::Table edges_table = db_->getTable(RPI_PERIPHERAL_EDGES_TABLE);

  const mysqlx::Result result = edges_table
      .insert("rpi_id", "peripheral_id")
      .values(1, rpi_id)
      .values(2, peripheral_id)
      .execute();

  if (result.getAffectedItemsCount() == 0)
  {
    LOG(ERROR) << "Failed to insert rpi-peripheral edge";
    return false;
  }

  return true;
}

bool DbManager::ContainsRpi(size_t id)
{
  return ContainsRecordById(db_.get(), RPIS_TABLE, id);
}

bool DbManager::ContainsPeripheral(const std::string &name)
{
  mysqlx::Table table = db_->getTable(PERIPHERALS_TABLE);

  size_t record_count =
      table.select()
          .where("name = :name")
          .bind(":name", name)
          .execute()
          .count();

  return record_count > 0;
}

bool DbManager::ContainsPeripheral(size_t id)
{
  return ContainsRecordById(db_.get(), RPI_PERIPHERAL_EDGES_TABLE, id);
}

bool DbManager::InsertPeripheral(const std::string &name, size_t *out_id)
{
  assert(out_id);

  mysqlx::Table peripherals_table = db_->getTable(PERIPHERALS_TABLE);

  const mysqlx::Result result = peripherals_table
      .insert("name", "time")
      .values(1, name)
      .values(2, MakeTimestamp())
      .execute();

  if (result.getAffectedItemsCount() == 0)
  {
    LOG(ERROR) << "Failed to insert peripheral";
    return false;
  }

  *out_id = result.getAutoIncrementValue();
  return true;
}

bool DbManager::InsertSoilMoistureSensor(
    const std::string& name,
    float floor,
    float ceil,
    size_t *out_id)
{
  LOG(INFO) << "Registering soil moisture sensor w/database, {name="
            << name << ", floor=" << floor << ", ceil=" << ceil << "}";

  try
  {
    session_->startTransaction();

    if (!InsertPeripheral(name, out_id))
    {
      LOG(ERROR) << "Failed to insert peripheral record";
      goto error;
    }

    mysqlx::Table table = db_->getTable(SOIL_MOISTURE_SENSORS_TABLE);

    const mysqlx::Result result = table
        .insert("peripheral_id", "ceiling", "floor")
        .values(1, *out_id)
        .values(2, ceil)
        .values(3, floor)
        .execute();

    if (result.getAffectedItemsCount() == 0)
    {
      LOG(ERROR) << "Failed to insert soil moisture sensor record";
      goto error;
    }

    LOG(INFO) << "Soil moisture sensor registered successfully";
    return true;
  }
  catch (...)
  {
    goto error;
  }

error:
    LOG(ERROR) << "Transaction failure when inserting soil moisture sensor. Rolling back...";
    session_->rollback();
    return false;
}

bool DbManager::InsertSoilMoistureMeasurement(
    size_t sensor_id,
    float measurement)
{
  mysqlx::Table table = db_->getTable(SOIL_MOISTURE_MEASUREMENTS_TABLE);
  const mysqlx::Result result = table
      .insert("sensor_id", "measurement", "timestamp")
      .values(1, sensor_id)
      .values(2, measurement)
      .values(3, MakeTimestamp())
      .execute();

  if (result.getAffectedItemsCount() == 0)
  {
    LOG(ERROR) << "Failed to insert soil moisture measurement record";
    return false;
  }

  return true;
}

bool DbManager::InsertRpi(
    const std::string &name,
    const std::string &location,
    size_t *out_id)
{
  assert(out_id);
  mysqlx::Table rpi_table = db_->getTable(RPIS_TABLE);

  const mysqlx::Result result = rpi_table
      .insert("name", "time", "location")
      .values(1, name)
      .values(2, MakeTimestamp())
      .values(3, location)
      .execute();

  if (result.getAffectedItemsCount() == 0)
  {
    LOG(ERROR) << "Failed to insert peripheral";
    return false;
  }

  *out_id = result.getAutoIncrementValue();
  return true;
}

void DbManager::CloseResources()
{
  is_initialized_ = false;
  session_->close();
  session_.reset();
  db_.reset();
}

void DbManager::StealResources(DbManager *other)
{
  assert(other);
  is_initialized_ = other->is_initialized_;
  other->is_initialized_ = false;
  session_ = std::move(other->session_);
  db_ = std::move(other->db_);
}

} // namespace organicdump
