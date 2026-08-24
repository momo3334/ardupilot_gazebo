/*
   Copyright (C) 2024 ardupilot.org

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SOCKETUDP_HH_
#define SOCKETUDP_HH_

#include <cstddef>
#include <cstdint>

#ifdef _WIN32

// winsock2.h must be reached before windows.h, otherwise the latter pulls in
// the incompatible Winsock 1.1 declarations. The build defines
// WIN32_LEAN_AND_MEAN globally so that windows.h never does so.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <basetsd.h>

#else

#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>

#endif

#ifdef _WIN32
/// \brief Native socket handle. Winsock uses an opaque unsigned handle rather
/// than a file descriptor, so it must never be tested against negative values.
using SocketHandle = SOCKET;

/// \brief Signed byte count returned by socket I/O; POSIX ssize_t equivalent.
using SocketSSize = SSIZE_T;

/// \brief Sentinel value for a socket that is not open.
inline constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
using SocketHandle = int;
using SocketSSize = ssize_t;
inline constexpr SocketHandle kInvalidSocket = -1;
#endif

/// \brief Simple UDP socket handling class.
class SocketUDP {
public:
    /// \brief Constructor.
    SocketUDP(bool reuseaddress, bool blocking);

    /// \brief Destructor.
    ~SocketUDP();

    /// \brief Bind socket to address and port.
    bool bind(const char *address, uint16_t port);

    /// \brief Set reuse address option.
    bool set_reuseaddress();

    /// \brief Set blocking state.
    bool set_blocking(bool blocking);

    /// \brief Send data to address and port.
    SocketSSize
    sendto(const void *buf, size_t size, const char *address, uint16_t port);

    /// \brief Receive data.
    SocketSSize recv(void *pkt, size_t size, uint32_t timeout_ms);

    /// \brief Get last client address and port
    void get_client_address(const char *&ip_addr, uint16_t &port);

private:
    /// \brief Address of the last datagram received.
    struct sockaddr_in in_addr{};

    /// \brief Socket handle.
    SocketHandle fd = kInvalidSocket;

    /// \brief Poll for incoming data with timeout.
    bool pollin(uint32_t timeout_ms);

    /// \brief Close the socket if it is open.
    void close_socket();

    /// \brief Make a sockaddr_in struct from address and port.
    void make_sockaddr(const char *address, uint16_t port,
                       struct sockaddr_in &sockaddr);
};

#endif  // SOCKETUDP_HH_
