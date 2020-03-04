#ifndef ORGANICDUMP_SERVER_PROTOBUFCLIENT_H
#define ORGANICDUMP_SERVER_PROTOBUFCLIENT_H

#include "organic_dump.pb.h"

#include "Fd.h"
#include "OrganicDumpProtoMessage.h"
#include "TlsConnection.h"

namespace organicdump
{

class ProtobufClient
{
public:
  ProtobufClient(network::TlsConnection cxn);
  ProtobufClient(
      network::TlsConnection cxn,
      organicdump_proto::ClientType type);
  bool Read(OrganicDumpProtoMessage *out_msg, bool *out_cxn_closed=nullptr);
  bool Write(OrganicDumpProtoMessage *msg, bool *out_cxn_closed=nullptr);
  const network::Fd &GetFd() const;
  const organicdump_proto::ClientType &GetType() const;
  size_t GetId() const;
  bool IsDifferentiated() const;
  void Differentiate(organicdump_proto::ClientType type, size_t id);

private:
  network::TlsConnection cxn_;
  organicdump_proto::ClientType type_;
  size_t id_;
};

} // namespace organicdump

#endif // ORGANICDUMP_SERVER_PROTOBUFCLIENT_H
