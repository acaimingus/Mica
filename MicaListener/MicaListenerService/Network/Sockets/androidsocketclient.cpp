/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Class for managing socket connections.
 */

#include "androidsocketclient.hpp"

namespace MicaListener::MicaListenerService::Network::Sockets
{
    AndroidSocketClient::AndroidSocketClient(const NetworkConfig &_config)
    {
        // Create the socket by type
        // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = IP
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
        {
            throw std::runtime_error("Could not create socket: " + std::string(std::strerror(errno)));
        }

        // Set 1s timeout
        timeval tv{};
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv));

        // Prepare the IP address
        sockaddr_in server{};
        server.sin_family = AF_INET;
        // Change port to network byte order (Endianness)
        server.sin_port = htons(_config.GetPort());

        // Change IP address from string to binary form
        if (inet_pton(AF_INET, _config.GetIp().c_str(), &server.sin_addr) <= 0)
        {
            close(sock);
            throw std::runtime_error("Invalid address / Address not supported: " + _config.GetIp());
        }

        // Connect
        std::clog << logName << "Connecting to " << _config.GetIp() << ":" << _config.GetPort() << "..." <<
                std::endl;
        if (connect(sock, reinterpret_cast<struct sockaddr *>(&server), sizeof(server)) < 0)
        {
            const std::string errorMsg = strerror(errno);
            // Clean up before throwing exception!
            close(sock);
            throw std::runtime_error("Connection failed: " + errorMsg);
        }
        std::clog << logName << "Connected!" << std::endl;
    }

    AndroidSocketClient::~AndroidSocketClient()
    {
        // Close the socket if it wasn't already
        if (sock != -1)
        {
            std::clog << logName << "Closing socket..." << std::endl;
            close(sock);
        }
    }

    ssize_t AndroidSocketClient::Read(std::vector<uint8_t> &_buffer) const
    {
        // Try reading data from the socket
        const ssize_t bytesRead = recv(sock, _buffer.data(), _buffer.size(), 0);

        // 0 bytes = Server closed connection
        if (bytesRead == 0)
        {
            std::clog << logName << "Server closed connection." << std::endl;
        } else if (bytesRead < 0)
        {
            // There was an error, check if it was a timeout
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // There was a timeout, no need to act
                return -1;
            }
            // An actual error happened
            std::cerr << logName << "Read error: " << strerror(errno) << std::endl;
        }
        return bytesRead;
    }
}
