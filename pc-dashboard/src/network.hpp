#pragma once

#include <cstdint>

namespace network {
// Disable compiler generated padding for now
#pragma pack(push, 1)

  struct ImuPacket {
    uint8_t magic;
  };
  struct LidarPacket {
    uint8_t magic;
  };
  struct CameraPacket {
    uint8_t magic;
  };
  struct ServoPacket {
    uint8_t magic;
    float angle;
  };

// Re-enable padding
#pragma pack(pop)

  enum class ConnectionState { Disconnected, Connecting, Connected };

  // Make sure that structure sizes are as expected (=match wire layout)
  static_assert(sizeof(ImuPacket) == 1, "IMU packet must match wire layout size");
  static_assert(sizeof(LidarPacket) == 1, "Lidar packet must match wire layout size");
  static_assert(sizeof(CameraPacket) == 1, "Camera packet must match wire layout size");
  static_assert(sizeof(ServoPacket) == 5, "Servo packet must match wire layout size");

  // UDP server to listen for telemetry + camera packets
  class Server {
  public:
  private:
  };

  // TCP client to send control commands, and init sending telemetry
  class Client {
  public:
    ConnectionState GetConnectionState();
    void Disconnect();
    void Connect();

  private:
    ConnectionState _connState = ConnectionState::Disconnected;
  };

} // namespace network