#ifndef ORGANICDUMP_SERVER_DBMANAGER_H
#define ORGANICDUMP_SERVER_DBMANAGER_H

#include <memory>

#include <mysqlx/xdevapi.h>

namespace organicdump
{

class DbManager {
public:
  static bool Create(DbManager *out_db);

public:
  DbManager();
  DbManager(
      std::unique_ptr<mysqlx::Session> session,
      std::unique_ptr<mysqlx::Schema> db);
  ~DbManager();
  DbManager(DbManager &&other);
  DbManager &operator=(DbManager &&other);
  bool OrphanRpiOwnedPeripheral(size_t peripheral_id);
  bool AssignPeripheralToRpi(size_t rpi_id, size_t peripheral_id);
  bool ContainsRpi(size_t id);
  bool ContainsRpi(const std::string &name);
  bool ContainsPeripheral(const std::string &name);
  bool ContainsPeripheral(size_t id);
  bool ContainsIrrigationSystem(size_t id);
  bool InsertRpi(
      const std::string &name,
      const std::string &location,
      size_t *out_id);
  bool InsertSoilMoistureSensor(
      const std::string& name,
      float floor,
      float ceil,
      size_t *out_id);
  bool InsertSoilMoistureMeasurement(
      size_t sensor_id,
      float measurement,
      size_t *out_measurement_id);
  bool UpdatePeripheralOwnership(
      size_t peripheral_id,
      size_t rpi_id);
  bool InsertIrrigationSystem(
      const std::string& name,
      size_t *out_id);
  bool InsertDailyIrrigationSchedule(
      size_t irrigation_system_id,
      size_t day_of_week_index,
      std::string water_time_military,
      size_t water_duration_ms);

private:
  void CloseResources();
  void StealResources(DbManager *other);
  bool InsertPeripheral(const std::string &name, size_t *out_id);
  bool DeletePeripheralOwnership(size_t peripheral_id);
  bool InsertPeripheralOwnership(size_t peripheral_id, size_t rpi_id);

private:
  DbManager(const DbManager &other) = delete;
  DbManager &operator=(const DbManager &other) = delete;

private:
  bool is_initialized_;
  std::unique_ptr<mysqlx::Session> session_;
  std::unique_ptr<mysqlx::Schema> db_;
};

} // namespace organicdump

#endif // ORGANICDUMP_SERVER_DBMANAGER_H
