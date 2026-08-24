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


#include "SocketUDP.hh"
#include <cstdio>
#include <cstdlib>
#include <cstring>


namespace {

#ifdef _WIN32
/// \brief Winsock has to be started up once per process before any socket
/// call, and shut down again when the last user goes away.
class WinsockContext {
public:
    WinsockContext() {
        WSADATA wsaData;
        started = WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    }

    ~WinsockContext() {
        if (started) {
            WSACleanup();
        }
    }

    bool started{false};
};
#endif

/// \brief Prepare the platform socket library for use.
void init_socket_library() {
#ifdef _WIN32
    static WinsockContext context;
    (void)context;
#endif
}

/// \brief Report the most recent socket error. Winsock does not set errno.
void report_socket_error(const char *what) {
#ifdef _WIN32
    std::fprintf(stderr, "%s: Winsock error %d\n", what, WSAGetLastError());
#else
    perror(what);
#endif
}

}  // namespace


SocketUDP::SocketUDP(bool reuseaddress, bool blocking) {
    init_socket_library();

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == kInvalidSocket) {
        report_socket_error("SocketUDP creation failed");
        exit(EXIT_FAILURE);
    }

#ifndef _WIN32
    // Windows does not support FD_CLOEXEC
    fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif

    if (reuseaddress) {
        set_reuseaddress();
    }
    if (blocking) {
        set_blocking(true);
    }
}


SocketUDP::~SocketUDP() {
    close_socket();
}


void SocketUDP::close_socket() {
    if (fd == kInvalidSocket) {
        return;
    }
#ifdef _WIN32
    ::closesocket(fd);
#else
    ::close(fd);
#endif
    fd = kInvalidSocket;
}


bool SocketUDP::bind(const char *address, uint16_t port) {
    struct sockaddr_in server_addr{};
    make_sockaddr(address, port, server_addr);

    if (::bind(fd, reinterpret_cast<sockaddr *>(&server_addr),
               static_cast<int>(sizeof(server_addr))) != 0) {
        report_socket_error("SocketUDP Bind failed");
        close_socket();
        return false;
    }
    return true;
}


bool SocketUDP::set_reuseaddress() {
    int one = 1;
    return (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                       reinterpret_cast<const char *>(&one),
                       static_cast<int>(sizeof(one))) != -1);
}


bool SocketUDP::set_blocking(bool blocking) {
#ifdef _WIN32
    u_long mode = blocking ? 0 : 1;
    return ioctlsocket(fd, FIONBIO, &mode) != SOCKET_ERROR;
#else
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    return fcntl(fd, F_SETFL, flags) != -1;
#endif
}


SocketSSize SocketUDP::sendto(const void *buf, size_t size,
                              const char *address, uint16_t port) {
    struct sockaddr_in sockaddr_out{};
    make_sockaddr(address, port, sockaddr_out);

    return ::sendto(fd, static_cast<const char *>(buf),
                    static_cast<int>(size), 0,
                    reinterpret_cast<sockaddr *>(&sockaddr_out),
                    static_cast<int>(sizeof(sockaddr_out)));
}

/*
  receive some data
 */
SocketSSize SocketUDP::recv(void *buf, size_t size, uint32_t timeout_ms) {
    if (!pollin(timeout_ms)) {
        return -1;
    }

#ifdef _WIN32
    // Winsock has no MSG_DONTWAIT. pollin() has already established that a
    // datagram is queued, so an ordinary recvfrom will not block here.
    const int flags = 0;
#else
    const int flags = MSG_DONTWAIT;
#endif

    socklen_t len = sizeof(this->in_addr);
    return ::recvfrom(fd, static_cast<char *>(buf), static_cast<int>(size),
                      flags, reinterpret_cast<sockaddr *>(&in_addr), &len);
}


void SocketUDP::get_client_address(const char *&ip_addr, uint16_t &port) {
    ip_addr = inet_ntoa(in_addr.sin_addr);
    port = ntohs(in_addr.sin_port);
}


bool SocketUDP::pollin(uint32_t timeout_ms) {
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    tv.tv_sec = static_cast<long>(timeout_ms / 1000);
    tv.tv_usec = static_cast<long>((timeout_ms % 1000) * 1000UL);

#ifdef _WIN32
    // Winsock ignores the descriptor count and takes it from the fd_set.
    const int nfds = 0;
#else
    const int nfds = fd + 1;
#endif

    if (select(nfds, &fds, nullptr, nullptr, &tv) != 1) {
        return false;
    }
    return true;
}

void SocketUDP::make_sockaddr(const char *address, uint16_t port,
                              struct sockaddr_in &sockaddr) {
    sockaddr = {};

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(address);
    sockaddr.sin_port = htons(port);
#ifdef HAVE_SOCK_SIN_LEN
    sockaddr.sin_len = sizeof(sockaddr);
#endif
}
