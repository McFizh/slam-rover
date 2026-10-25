#include "network.hpp"

#include <cstdio>
#include <cstring>
#include <fcntl.h>

#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * UDP telemetry server
 */
namespace network {
  Server::~Server() { Stop(); }

  bool Server::GetLatestFrame(std::vector<uint8_t> &out) {
    std::lock_guard<std::mutex> lock(_latestFrameMutex);

    if (!_newFrameReady)
      return false;

    out.swap(_latestFrameBuf);
    _newFrameReady = false;

    return true;
  }

  void Server::ProcessCameraPacket(uint8_t *buf, ssize_t numBytes) {
    const int hdrSize = sizeof(CameraPacket);
    const int framePayloadSize = numBytes - hdrSize;

    // Invalid payload size
    if (framePayloadSize <= 0 || framePayloadSize > MAX_CAMERA_PAYLOAD_SIZE)
      return;

    // Validate header
    CameraPacket header;
    std::memcpy(&header, buf, hdrSize);

    if (header.chunk_cnt == 0 || header.chunk_idx >= header.chunk_cnt) {
      return;
    }

    // New frame?
    if (!_frameInProgress || _currentFrameId != header.frame_id) {
      if (_frameInProgress) {
        std::printf("Incomplete frame %u (got %u chunks)\n", _currentFrameId, _nextChunkIdx);
      }

      // Skip this frame, if it starts from other chunk idx than 0
      if (header.chunk_idx != 0)
        return;

      _frameBuf.clear();
      _frameInProgress = true;
      _currentFrameId = header.frame_id;
      _nextChunkIdx = 0;
    }

    // Ignore rest of frames chunks, if current chunk is the wrong one
    if (header.chunk_idx != _nextChunkIdx) {
      _frameInProgress = false;
      return;
    }

    // Store the chunk to buffer
    _frameBuf.insert(_frameBuf.end(), buf + hdrSize, buf + numBytes);
    _nextChunkIdx++;

    // This was the last chunk
    if (_nextChunkIdx == header.chunk_cnt) {
      std::lock_guard<std::mutex> lock(_latestFrameMutex);
      _latestFrameBuf.swap(_frameBuf);
      _newFrameReady = true;
      _frameInProgress = false;
    }
  }

  void Server::ProcessTelemetryPacket(uint8_t *buf, ssize_t numBytes) {
    uint8_t magic = buf[0];
    uint8_t msgType = buf[1];

    // If packet doesn't start with magic number, then it can be ignored
    if (magic != WIRE_MAGIC)
      return;

    // Camera telemetry packet
    if (msgType == MSGTYPE_CAMERA) {
      ProcessCameraPacket(buf, numBytes);
    }
  }

  void Server::Worker() {
    pollfd pfds[1] = {{_serverFd, POLLIN, 0}};
    uint8_t buf[2048];

    while (_serverRunning) {
      // Wait 150ms for incoming udp packets
      int poll_status = poll(pfds, 1, 150);
      if (poll_status < 0) {
        // EINTR = Interrupted system call, we can safely ignore this
        if (errno == EINTR)
          continue;
        break;
      }

      // Poll timeout, nothing received
      if (poll_status == 0)
        continue;

      // Receive (and handle) the packet
      ssize_t numBytes = recvfrom(_serverFd, buf, sizeof(buf), 0, nullptr, nullptr);
      if (numBytes > 1)
        ProcessTelemetryPacket(buf, numBytes);
    }

    _serverRunning = false;
  }

  int Server::CreateListeningUdpSocket(int port) {
    _serverFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (_serverFd < 0)
      return -1;

    // Set the socket to non-blocking state (we'll handle this with poll in the thread)
    int flags = fcntl(_serverFd, F_GETFL, 0);
    fcntl(_serverFd, F_SETFL, flags | O_NONBLOCK);

    // Start listening for packets
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(_serverFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
      close(_serverFd);
      _serverFd = -1;
      return -1;
    }

    return 0;
  }

  int Server::Start() {
    if (_serverRunning)
      return 0;

    // Call stop just in case the worker previously bailed out with an error
    Stop();

    // Create UDP socket
    if (CreateListeningUdpSocket(5001) == -1)
      return -1;

    // Socket created, start the actual thread to handle incoming packages
    _serverRunning = true;
    _serverThread = std::thread(&Server::Worker, this);
    return 0;
  }

  void Server::Stop() {
    _serverRunning = false;

    if (_serverThread.joinable())
      _serverThread.join();

    if (_serverFd >= 0) {
      close(_serverFd);
      _serverFd = -1;
    }
  }

} // namespace network