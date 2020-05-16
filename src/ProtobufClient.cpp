#include "ProtobufClient.h"

#include <utility>

#include "organic_dump.pb.h"

#include "Fd.h"
#include "OrganicDumpProtoMessage.h"

namespace
{
using organicdump_proto::ClientType;
using network::TlsConnection;
} // namespace

namespace organicdump
{

ProtobufClient::ProtobufClient(TlsConnection cxn)
  : cxn_{std::move(cxn)},
    type_{ClientType::UNKNOWN},
    id_{} {}

bool ProtobufClient::Read(OrganicDumpProtoMessage *out_msg, bool *out_cxn_closed)
{
  assert(out_msg);

  if (!ReadTlsProtobufMessage(
        &cxn_,
        out_msg,
        out_cxn_closed))
  {
    if (out_cxn_closed)
    {
      return true;
    }

    LOG(ERROR) << "Failed to read TLS protobuf message";
    return false;
  }

  return true;
}

bool ProtobufClient::Write(OrganicDumpProtoMessage *msg, bool *out_cxn_closed)
{
  assert(msg);

  if (!SendTlsProtobufMessage(
        &cxn_,
        msg,
        out_cxn_closed))
  {
    LOG(ERROR) << "Failed to write TLS protobuf message";
    return false;
  }

  return true;
}

const network::Fd &ProtobufClient::GetFd() const
{
  return cxn_.GetFd();
}

const organicdump_proto::ClientType &ProtobufClient::GetType() const
{
  return type_;
}

size_t ProtobufClient::GetId() const
{
  return id_;
}

bool ProtobufClient::IsDifferentiated() const
{
  return type_ != ClientType::UNKNOWN;
}

void ProtobufClient::Differentiate(organicdump_proto::ClientType type, size_t id)
{
  assert(!IsDifferentiated());
  assert(type != ClientType::UNKNOWN);
  type_ = type;
  id_ = id;
}

} // namespace organicdump
