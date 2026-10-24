#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include <sys/types.h>

#define WIRE_MAGIC 0xC4
#define MSGTYPE_CAMERA 1
#define MAX_CAMERA_PAYLOAD_SIZE 1300

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
    uint8_t msg_type;
    uint32_t frame_id;
    uint16_t chunk_idx;
    uint16_t chunk_cnt;
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
  static_assert(sizeof(CameraPacket) == 10, "Camera packet must match wire layout size");
  static_assert(sizeof(ServoPacket) == 5, "Servo packet must match wire layout size");

  // UDP server to listen for telemetry + camera packets
  class Server {
  public:
    ~Server();
    int Start();
    void Stop();
    bool GetLatestFrame(std::vector<uint8_t> &out);

  private:
    void Worker();
    int CreateListeningUdpSocket(int port);
    void ProcessTelemetryPacket(uint8_t *buf, ssize_t numBytes);
    void ProcessCameraPacket(uint8_t *buf, ssize_t numBytes);

    std::thread _serverThread;
    std::atomic<bool> _serverRunning{false};

    bool _frameInProgress = false;
    bool _newFrameReady = false;
    uint32_t _currentFrameId = 0;
    uint16_t _nextChunkIdx = 0;

    std::vector<uint8_t> _frameBuf;
    std::vector<uint8_t> _latestFrameBuf;
    std::mutex _latestFrameMutex;

    int _serverFd = -1;
  };

  // TCP client to send control commands, and init sending telemetry
  class Client {
  public:
    ConnectionState GetConnectionState();
    void Disconnect();
    void Connect();

  private:
    int ResolveHost();
    ConnectionState _connState = ConnectionState::Disconnected;
  };

} // namespace network