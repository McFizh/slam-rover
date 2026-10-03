#include "network.hpp"

namespace network {
  void Client::Connect() {}

  void Client::Disconnect() {}

  ConnectionState Client::GetConnectionState() { return _connState; }
} // namespace network