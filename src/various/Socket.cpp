/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdexcept>
#include <iostream>
#include <cstring>
#include <chrono>
#include "various/Socket.h"

#if defined(_WIN32)
class SocketInitialiseWrapper {
    public:
        SocketInitialiseWrapper() {
            WSADATA wsa;
            std::clog << "RTL_TCP_CLIENT: Initialising Winsock...";

            if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
                std::clog << "Failed. Error Code :" << WSAGetLastError();
            std::clog << "done" << std::endl;
        }

        ~SocketInitialiseWrapper() {
            WSACleanup();
        }

        SocketInitialiseWrapper(SocketInitialiseWrapper&) = delete;
        SocketInitialiseWrapper& operator=(SocketInitialiseWrapper&) = delete;
};

static SocketInitialiseWrapper socketInitialiseWrapper;
#endif

Socket::~Socket()
{
    if (valid()) {
        close();
    }
}

Socket::Socket(Socket&& other)
{
    if (&other == this) {
        return;
    }
    sock = other.sock;
    other.sock = INVALID_SOCKET;
}

Socket& Socket::operator=(Socket&& other)
{
    if (&other != this) {
        close();
        sock = other.sock;
        other.sock = INVALID_SOCKET;
    }
    return *this;
}

void Socket::close()
{
    if (!valid()) return;
#if defined(_WIN32)
    closesocket(sock);
#else
    ::close(sock);
#endif

    sock = INVALID_SOCKET;
}

bool Socket::valid() const
{
    return sock != (int) INVALID_SOCKET;
}

ssize_t Socket::recv(void *buffer, size_t length, int flags)
{
    return ::recv(sock, (char*)buffer, length, flags);
}

ssize_t Socket::send(const void *buffer, size_t length, int flags)
{
    return ::send(sock, (const char*)buffer, length, flags);
}

bool Socket::bind(int port)
{
    if (valid()) {
        return false;
    }

    int listensock = ::socket(PF_INET, SOCK_STREAM, 0);

    if (listensock == -1) {
        perror("Could not create socket");
        return false;
    }

    int reuse = 1;
#if defined(_WIN32)
    if (setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse))
#else
    if (setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))
#endif
            == -1) {
        throw std::runtime_error("Can't reuse address");
    }

    sockaddr_in addr = {};
    addr.sin_family = PF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    const int bind_ret = ::bind(listensock, (sockaddr*)&addr, sizeof(sockaddr_in));
    if (bind_ret == -1) {
        perror("Could not bind socket");
#if defined(_WIN32)
        closesocket(listensock);
#else
        ::close(listensock);
#endif
        return false;
    }

    sock = listensock;
    return true;
}

bool Socket::listen()
{
    const int listen_ret = ::listen(sock, 64);
    if (listen_ret == -1) {
        perror("Could not listen");
        return false;
    }
    return true;
}

Socket Socket::accept()
{
    struct sockaddr_in remote_addr;
    socklen_t remote_addr_len = sizeof(remote_addr);
    int conn = ::accept(sock, (sockaddr*)&remote_addr, &remote_addr_len);
    if (conn == -1) {
        if (errno == ECONNABORTED) {
            return {};
        }
        perror("accept failed");
        return {};
    }

    Socket s;
    s.sock = conn;
    return s;
}

bool Socket::connect(const std::string& address, int port, int timeout)
{
#if defined(_WIN32)
    TIMEVAL Timeout;
#else
    struct timeval Timeout;
#endif
    Timeout.tv_sec = timeout;
    Timeout.tv_usec = 0;

    bool ret = false;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    struct addrinfo *result;

    std::string port_str = std::to_string(port);

    int s = getaddrinfo(address.c_str(), port_str.c_str(), &hints, &result);
    if (s != 0) {
#if defined(_WIN32)
        char * ch_errstr = gai_strerrorA(s);
#else
        const char * ch_errstr = gai_strerror(s);
#endif
        std::string errstr(ch_errstr);
        throw std::runtime_error("getaddrinfo: " + errstr);
    }

    struct addrinfo *rp;

    int sfd = -1;

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);

        if (sfd == -1)
            continue;

        // set the socket in non-blocking mode
#ifdef _WIN32
        unsigned long mode = 1;
        int iResult = ioctlsocket(sfd, FIONBIO, &mode);
#else
        int oldflags = fcntl(sfd, F_GETFL, 0);
        if (oldflags == -1) goto out;
        int flags = oldflags | O_NONBLOCK;
        int iResult = fcntl(sfd, F_SETFL, flags);
#endif
        if (iResult != 0)
        {
            std::clog << "Socket: Failed to put socket into non-blocking mode with error: " << iResult << std::endl;
        }

        struct sockaddr_in *sa = (struct sockaddr_in *) rp->ai_addr;
        std::clog << "Try to connect to: " << inet_ntoa(sa->sin_addr) << std::endl;
        ::connect(sfd, rp->ai_addr, rp->ai_addrlen);

        // set the socket back in blocking mode
#ifdef _WIN32
        mode = 0;
        iResult = ioctlsocket(sfd, FIONBIO, &mode);
#else
        iResult = fcntl(sfd, F_SETFL, oldflags);
#endif
        if (iResult != 0)
        {
             std::clog << "Socket: Failed to put socket into blocking mode with error: " << iResult << std::endl;
        }

        fd_set Write;
        FD_ZERO(&Write);
        FD_SET(sfd, &Write);

        // check if the socket is ready
        int sel_value = ::select(sfd+1, nullptr, &Write, nullptr, &Timeout);
        if(FD_ISSET(sfd, &Write) && sel_value > 0)
        {
            int error=0; socklen_t size=sizeof(error);
            iResult = 0;

#ifndef _WIN32
            iResult = ::getsockopt(sfd, SOL_SOCKET, SO_ERROR, &error, &size);
#endif
            if(error > 0 && iResult == 0)
            {
                std::clog << "Socket: Connection failed: \"" << strerror(error) << "\"" << std::endl;
            }
            else
            {
                sock = sfd;
                break; /* Success */
            }
        }

#if defined(_WIN32)
        closesocket(sfd);
#else
        ::close(sfd);
#endif
    }

    if (rp == NULL) {               /* No address succeeded */
        std::clog << "Socket: Could not connect" << std::endl;
        goto out;
    }

    ret = true;

out:
    freeaddrinfo(result);           /* No longer needed */

    return ret;
}

// HTTP callers set nonblocking mode before using bounded output.
bool Socket::setNonBlocking()
{
#ifdef _WIN32
    unsigned long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(sock, F_GETFL, 0);
    return flags >= 0 && fcntl(sock, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

bool Socket::waitReadable(int timeout_ms) const
{
    if (!valid()) return false;
#ifndef _WIN32
    if (sock >= FD_SETSIZE) return false;
#endif
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    return select(sock + 1, &fds, nullptr, nullptr, &tv) > 0;
}

bool Socket::peerClosed() const
{
    if (!valid()) return true;
    if (!waitReadable(0)) return false;
    char byte;
    const auto n = ::recv(sock, &byte, 1, MSG_PEEK);
    if (n == 0) return true;
#ifdef _WIN32
    return n < 0 && WSAGetLastError() != WSAEWOULDBLOCK;
#else
    return n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR;
#endif
}

ssize_t Socket::sendAll(const void *buffer, size_t length, int flags, int timeout_ms)
{
    using namespace std::chrono;
    const auto deadline = steady_clock::now() + milliseconds(timeout_ms);
    size_t sent = 0;
    while (sent < length) {
        const auto remaining = duration_cast<milliseconds>(deadline - steady_clock::now()).count();
        if (!valid() || remaining <= 0) return -1;
#ifndef _WIN32
        if (sock >= FD_SETSIZE) return -1;
#endif
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        timeval tv{static_cast<long>(remaining / 1000), static_cast<long>((remaining % 1000) * 1000)};
        const int ready = select(sock + 1, nullptr, &fds, nullptr, &tv);
        if (ready == 0) return -1;
        if (ready < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        const auto n = send(static_cast<const char*>(buffer) + sent, length - sent, flags);
        if (n > 0) sent += n;
        else if (n == 0) return -1;
        else {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) continue;
#else
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
#endif
            return -1;
        }
    }
    return static_cast<ssize_t>(sent);
}
