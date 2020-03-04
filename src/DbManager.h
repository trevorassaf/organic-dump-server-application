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
      float measurement);

private:
  void CloseResources();
  void StealResources(DbManager *other);
  bool InsertPeripheral(const std::string &name, size_t *out_id);

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
