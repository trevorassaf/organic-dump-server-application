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
constexpr const char *SOIL_MOISTURE_MEASUREMENTS_TABLE = "soil_moisture_readings";
constexpr const char *IRRIGATION_SYSTEMS_TABLE = "irrigation_systems";
constexpr const char *DAILY_IRRIGATION_SCHEDULES_TABLE = "daily_irrigation_schedules";

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
      table.select("id")
          .where("id = :key")
          .bind("key", id)
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
      .bind("id", peripheral_id)
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
      .values(rpi_id, peripheral_id)
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

bool DbManager::ContainsRpi(const std::string &name)
{
  mysqlx::Table table = db_->getTable(RPIS_TABLE);
  size_t record_count =
      table.select("name")
          .where("name = :rpi_name")
          .bind("rpi_name", name)
          .execute()
          .count();

  return record_count > 0;
}

bool DbManager::ContainsPeripheral(const std::string &name)
{
  mysqlx::Table table = db_->getTable(PERIPHERALS_TABLE);

  size_t record_count =
      table.select("id")
          .where("name = :name")
          .bind("name", name)
          .execute()
          .count();

  return record_count > 0;
}

bool DbManager::ContainsPeripheral(size_t id)
{
  return ContainsRecordById(db_.get(), PERIPHERALS_TABLE, id);
}

bool DbManager::ContainsIrrigationSystem(size_t id) {
  return ContainsRecordById(db_.get(), IRRIGATION_SYSTEMS_TABLE, id);
}

bool DbManager::InsertPeripheral(const std::string &name, size_t *out_id)
{
  assert(out_id);

  mysqlx::Table peripherals_table = db_->getTable(PERIPHERALS_TABLE);

  const mysqlx::Result result = peripherals_table
      .insert("name", "time")
      .values(name, MakeTimestamp())
      .execute();

  if (result.getAffectedItemsCount() == 0)
  {
    LOG(ERROR) << "Failed to insert peripheral";
    return false;
  }

  *out_id = result.getAutoIncrementValue();
  return true;
}

bool DbManager::DeletePeripheralOwnership(size_t peripheral_id)
{
  try
  {
    mysqlx::Table rpi_peripheral_edges =
        db_->getTable(RPI_PERIPHERAL_EDGES_TABLE);

    const mysqlx::Result result = rpi_peripheral_edges
      .remove()
      .where("peripheral_id = :pid")
      .bind("pid", peripheral_id)
      .execute();

    LOG(INFO) << "Removed " << result.getAffectedItemsCount()
              << " from " << RPI_PERIPHERAL_EDGES_TABLE;
  }
  catch (const mysqlx::Error e)
  {
    LOG(ERROR) << "Failed to remove record from " << RPI_PERIPHERAL_EDGES_TABLE
               << ". Error: " << e;
    return false;
  }

  return true;
}

bool DbManager::InsertPeripheralOwnership(
    size_t peripheral_id,
    size_t rpi_id)
{
  try
  {
    mysqlx::Table rpi_peripheral_edges =
        db_->getTable(RPI_PERIPHERAL_EDGES_TABLE);

    const mysqlx::Result result = rpi_peripheral_edges
      .insert("peripheral_id", "rpi_id")
      .values(peripheral_id, rpi_id)
      .execute();

    LOG(INFO) << "Inserted " << result.getAffectedItemsCount() << " rows into "
              << RPI_PERIPHERAL_EDGES_TABLE;

    return true;
  }
  catch (const mysqlx::Error e)
  {
    LOG(ERROR) << "Failed to insert record into " << RPI_PERIPHERAL_EDGES_TABLE
               << ". Error: " << e;
    return false;
  }
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
        .values(*out_id, ceil, floor)
        .execute();

    if (result.getAffectedItemsCount() == 0)
    {
      LOG(ERROR) << "Failed to insert soil moisture sensor record";
      goto error;
    }

    LOG(INFO) << "Soil moisture sensor registered successfully";
    session_->commit();
    return true;
  }
  catch (const mysqlx::Error e)
  {
    LOG(ERROR) << e;
    goto error;
  }

error:
    LOG(ERROR) << "Transaction failure when inserting soil moisture sensor. Rolling back...";
    session_->rollback();
    return false;
}

bool DbManager::UpdatePeripheralOwnership(size_t peripheral_id, size_t rpi_id) {
  // Check peripheral exists
  if (!ContainsPeripheral(peripheral_id)) {
    LOG(ERROR) << "Could not find peripheral w/id: " << peripheral_id;
    return false;
  }

  // Check RPI exists
  if (!ContainsRpi(rpi_id)) {
    LOG(ERROR) << "Could not find rpi w/id: " << rpi_id;
    return false;
  }

  try
  {
    session_->startTransaction();

    // Try to ownership record
    if (!DeletePeripheralOwnership(peripheral_id))
    {
      LOG(ERROR) << "Failed to delete peripheral ownership record. Id: "
                 << peripheral_id;
      goto error;
    }

    // Insert new ownership record
    if (!InsertPeripheralOwnership(peripheral_id, rpi_id))
    {
      LOG(ERROR) << "Failed to insert peripheral ownership record"
                 << ". Peripheral ID: " << peripheral_id
                 << ". RPI ID: " << rpi_id;
      goto error;
    }

    LOG(INFO) << "Peripheral ownership updated successfully!";
    session_->commit();
    return true;
  }
  catch (const mysqlx::Error &e)
  {
    LOG(ERROR) << e;
    goto error;
  }

error:
  LOG(ERROR) << "Transaction failure when updating peripheral paranetage. Rolling back...";
  session_->rollback();
  return false;
}

bool DbManager::InsertSoilMoistureMeasurement(
    size_t sensor_id,
    float measurement,
    size_t *out_measurement_id)
{
  assert(out_measurement_id);

  try
  {
    auto now = MakeTimestamp();
    LOG(INFO) << "Inserting soil moisture reading: sensor_id="
              << sensor_id << ", reading=" << measurement << ", time="
              << now;

    mysqlx::Table table = db_->getTable(SOIL_MOISTURE_MEASUREMENTS_TABLE);
    const mysqlx::Result result = table
        .insert("sensor_id", "reading", "time")
        .values(sensor_id, measurement, now)
        .execute();

    if (result.getAffectedItemsCount() == 0)
    {
      LOG(ERROR) << "Failed to insert soil moisture measurement record";
      return false;
    }

    *out_measurement_id = result.getAutoIncrementValue();
    LOG(INFO) << "Successfully inserted soil moisture reading. Id=" << *out_measurement_id;

    return true;
  }
  catch (const mysqlx::Error e)
  {
    LOG(ERROR) << "Failed to insert into " << SOIL_MOISTURE_MEASUREMENTS_TABLE
               << ". Error: " << e;
    return false;
  }
}

bool DbManager::InsertRpi(
    const std::string &name,
    const std::string &location,
    size_t *out_id)
{
  try
  {
    assert(out_id);
    mysqlx::Table rpi_table = db_->getTable(RPIS_TABLE);

    const mysqlx::Result result = rpi_table
        .insert("name", "time", "location")
        .values(name, MakeTimestamp(), location)
        .execute();

    if (result.getAffectedItemsCount() == 0)
    {
      LOG(ERROR) << "Failed to insert peripheral";
      return false;
    }

    *out_id = result.getAutoIncrementValue();
    return true;
  }
  catch (const mysqlx::Error e)
  {
    LOG(ERROR) << "Failed to insert into " << RPIS_TABLE << ". Error: " << e;
    return false;
  }
}

bool DbManager::InsertIrrigationSystem(
    const std::string& name,
    size_t *out_id)
{
  assert(out_id);

  LOG(INFO) << "Registering irrigation system w/database: name="
            << name;
  try
  {
    session_->startTransaction();

    if (!InsertPeripheral(name, out_id))
    {
      LOG(ERROR) << "Failed to insert peripheral record";
      goto error;
    }

    mysqlx::Table table = db_->getTable(IRRIGATION_SYSTEMS_TABLE);

    const mysqlx::Result result = table
        .insert("peripheral_id")
        .values(*out_id)
        .execute();

    if (result.getAffectedItemsCount() == 0)
    {
      LOG(ERROR) << "Failed to insert irrigation systems record";
      goto error;
    }

    LOG(INFO) << "Irrigation system " << *out_id << " registered successfully";
    session_->commit();
    return true;
  }
  catch (const mysqlx::Error e)
  {
    LOG(ERROR) << e;
    goto error;
  }

error:
    LOG(ERROR) << "Transaction failure when inserting irrigation system. Rolling back...";
    session_->rollback();
    return false;
}

bool DbManager::InsertDailyIrrigationSchedule(
    size_t irrigation_system_id,
    size_t day_of_week_index,
    std::string water_time_military,
    size_t water_duration_ms)
{
  LOG(INFO) << "Inserting daily irrigation schedule:"
            << " irrigation_system_id=" << irrigation_system_id
            << " day_of_week_index=" << day_of_week_index
            << " water_time_=" << water_time_military
            << " water_duration_ms=" << water_duration_ms;
  try
  {
    session_->startTransaction();
    mysqlx::Table table = db_->getTable(DAILY_IRRIGATION_SCHEDULES_TABLE);

    const mysqlx::Result result = table
        .insert(
              "irrigation_system_id",
              "day_of_week_index",
              "water_time_military",
              "water_duration_ms")
        .values(
              irrigation_system_id,
              day_of_week_index,
              water_time_military,
              water_duration_ms)
        .execute();

    if (result.getAffectedItemsCount() == 0)
    {
      LOG(ERROR) << "Failed to insert daily irrigation system record";
      goto error;
    }

    size_t schedule_id = result.getAutoIncrementValue();
    LOG(INFO) << "Daily irrigation schedule " << schedule_id << " inserted successfully";
    session_->commit();
    return true;
  }
  catch (const mysqlx::Error e)
  {
    LOG(ERROR) << e;
    goto error;
  }

error:
    LOG(ERROR) << "Transaction failure when inserting daily irrigation "
               << "schedule. Rolling back...";
    session_->rollback();
    return false;
}

void DbManager::CloseResources()
{
  if (!is_initialized_)
  {
    return;
  }

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
