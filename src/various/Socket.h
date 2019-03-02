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

#pragma once

#include <cstddef>
#include <string>

#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>

    #ifndef MSG_NOSIGNAL
    # define MSG_NOSIGNAL 0
    #endif
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <sys/wait.h>
    #include <sys/types.h>
    #include <sys/time.h>
    #include <fcntl.h>

    #define INVALID_SOCKET (-1)
#endif

class Socket {
    public:
        Socket() = default;
        ~Socket();
        Socket(const Socket& other) = delete;
        Socket& operator=(const Socket& other) = delete;
        Socket(Socket&& other);
        Socket& operator=(Socket&& other);

        void close();
        bool valid() const;

        // Binds to any address
        bool bind(int port);
        bool listen();
        Socket accept();
        bool connect(const std::string& address, int port, int timeout);

        ssize_t recv(void *buffer, size_t length, int flags);
        ssize_t send(const void *buffer, size_t length, int flags);

    private:
        int sock = INVALID_SOCKET;
};
