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
        sock = other.sock;
        other.sock = INVALID_SOCKET;
    }
    return *this;
}

void Socket::close()
{
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

    sockaddr_in addr;
    addr.sin_family = PF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    const int bind_ret = ::bind(listensock, (sockaddr*)&addr, sizeof(sockaddr_in));
    if (bind_ret == -1) {
        perror("Could not bind socket");
        return false;
    }

    sock = listensock;
    return true;
}

bool Socket::listen()
{
    const int listen_ret = ::listen(sock, 1);
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

bool Socket::connect(const std::string& address, int port)
{
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

        if (::connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            sock = sfd;
            break;                  /* Success */
        }
#if defined(_WIN32)
        closesocket(sfd);
#else
        ::close(sfd);
#endif
    }

    if (rp == NULL) {               /* No address succeeded */
        std::clog << "Could not connect" << std::endl;
        goto out;
    }

    ret = true;

out:
    freeaddrinfo(result);           /* No longer needed */

    return ret;
}
