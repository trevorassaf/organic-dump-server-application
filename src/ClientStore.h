#ifndef ORGANICDUMP_SERVER_CLIENTSTORE_H
#define ORGANICDUMP_SERVER_CLIENTSTORE_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "ProtobufClient.h"

namespace organicdump
{

class ClientStore
{
public:
  using FdInt = int;
  using IrrigationSystemId = int;

public:
  ClientStore();
  ClientStore(ClientStore &&other);
  ClientStore &operator=(ClientStore &&other);
  ~ClientStore();

  void Add(ProtobufClient client);
  void Remove(FdInt fd);
  void RemoveAll();

  std::vector<FdInt> GetFds() const;
  ProtobufClient* GetClient(FdInt fd);
  ProtobufClient* GetIrrigationSystemClient(IrrigationSystemId id);
  bool HasIrrigationSystemClient(IrrigationSystemId id);

private:
  void StealResources(ClientStore *other);

private:
  ClientStore(const ClientStore &other) = delete;
  ClientStore &operator=(const ClientStore &other) = delete;

private:
  std::unordered_map<FdInt, ProfobufClient> clients_;
  std::unordered_map<IrrigationSystemId, FdInt> irrigation_system_id_to_fd_table_;
  std::unordered_map<FdInt, IrrigationSystemInt> irrigation_system_fd_to_id_table_;
};

}; // namespace organicdump

#endif // ORGANICDUMP_SERVER_CLIENTSTORE_H
