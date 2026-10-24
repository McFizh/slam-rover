#include "network.hpp"

#include <cstdio>
#include <fcntl.h>
#include <cstring>
#include <string>

#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr const char *ROVER_HOSTNAME = "slamrover.local";

/**
 * TCP client
 */
namespace network {
  /**
   * Try to resolve the ip of rover using mDNS name
   */
  int Client::ResolveHost() {
    // Both ip4/ip6 will do, but socket type has to be TCP
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = std::to_string(5000);

    // Run search .. res = 0 ok, otherwise failure
    addrinfo *result = nullptr;
    int res = getaddrinfo(ROVER_HOSTNAME, portStr.c_str(), &hints, &result);
    if (res != 0) {
      std::fprintf(stderr, "getaddrinfo(%s) failed: %s\n", ROVER_HOSTNAME, gai_strerror(res));
      return -1;
    }

    // Go through results and try to connect
    int fd = -1;
    for (addrinfo *itr = result; itr != nullptr; itr = itr->ai_next) {
      fd = socket(itr->ai_family, itr->ai_socktype, itr->ai_protocol);
      if (fd < 0)
        continue;

      // Set socket to non-blocking state and try to connect
      int flags = fcntl(fd, F_GETFL, 0);
      fcntl(fd, F_SETFL, flags | O_NONBLOCK);

      int connRes = connect(fd, itr->ai_addr, itr->ai_addrlen);

      // Connected immediately (though most likely won't happen)
      if (connRes == 0)
        break;

      // EINPROGRESS just means that connection is still happening (due to socket being non-blocking)
      if (errno != EINPROGRESS) {
        close(fd);
        fd = -1;
        continue;
      }

      // Start polling for connection status (in 500ms slices, max 5000ms)
      int timeSpent = 0;
      int pollRes = 0;
      while (timeSpent < 5000) {
        pollfd pfd{fd, POLLOUT, 0};
        pollRes = poll(&pfd, 1, 500);

        // res != 0 -> connected or failed
        if (pollRes != 0)
          break;

        timeSpent += 500;
      }

      // Connection failed
      if (pollRes <= 0) {
        close(fd);
        fd = -1;
        continue;
      }

      // pollRes > 0 doesn't mean that socket is functional, we can reach this point also when connection is refused
      int socketErr = 0;
      socklen_t len = sizeof(socketErr);
      if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socketErr, &len) < 0 || socketErr != 0) {
        close(fd);
        fd = -1;
        continue;
      }

      // Finally we have a connection
      break;
    }

    // Release results
    freeaddrinfo(result);
    return fd;
  }

  void Client::Connect() {
    if (_connState == ConnectionState::Connected || _connState == ConnectionState::Connecting)
      return;

    _connState = ConnectionState::Connecting;
    if (ResolveHost() == -1) {
      _connState = ConnectionState::Disconnected;
      return;
    }

    _connState = ConnectionState::Connected;
  }

  void Client::Disconnect() {
    if (_connState == ConnectionState::Disconnected || _connState == ConnectionState::Connecting)
      return;
  }

  ConnectionState Client::GetConnectionState() { return _connState; }
} // namespace network