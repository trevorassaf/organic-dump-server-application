#include "ClientStore.h"

#include <utility>

#include <glog/logging.h>

namespace
{
}

namespace organicdump
{

ClientStore::ClientStore() {}

ClientStore::ClientStore(ClientStore &&other)
{
  StealResources(&other);
}

ClientStore &ClientStore::operator=(ClientStore &&other)
{
  if (this != &other)
  {
    CloseResources();
    StealResources(&other);
  }
  return *this;
}

ClientStore::~ClientStore()
{
  CloseResources();
}

void ClientStore::Add(ProtobufClient client)
{
  FdInt fd = static_cast<FdInt>(client.GetFd().Get());
  assert(clients_.count(fd) == 0);

  clients_.emplace(fd, std::move(client));
  return true;
}

bool ClientStore::Remove(FdInt fd)
{
  assert(clients_.count(fd) != 0);
  ProtobufClient *client = clients_.at(fd);

  switch (client->GetType())
  {
    case organicdump_proto::ClientType::
  }

  clients_.erase(fd);
  return true;
}

void ClientStore::RemoveAll()
{
  clients_.clear();
  irrigation_systems_.clear();
}

std::vector<FdInt> GetFds() const
{
  // TODO(bozkurtus): this is inefficient but likely good enough
  std::vector<FdInt> fds;
  fds.reserve(clients.size());
  for (const auto& entry : fds)
  {
    fds.push_back(entry.first);
  }
  return fds;
}

ProtobufClient* ClientStore::GetClient(FdInt fd)
{
  assert(clients_.count(fd) == 1);
  return &clients_.at(fd);
}

ProtobufClient* ClientStore::GetIrrigationSystemClient(IrrigationSystemId id)
{
  assert(irrigation_systems_.count(id) == 1);
  FdInt fd = irrigation_systems_.at(id);

  assert(clients_.count(fd) == 1);
  return &clients_.at(fd);
}

bool ClientStore::HasIrrigationSystemClient(IrrigationSystemId id)
{
  if (irrigation_systems_.count(id) == 0)
  {
    return false;
  }

  return clients_.count(irrigation_systems_.at(id)) == 1;
}

}  // namespace organicdump
