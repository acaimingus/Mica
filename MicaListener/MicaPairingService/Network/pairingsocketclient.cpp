/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Implementation of Unix domain socket client for communicating pairing decisions to MicaListener.
 */

#include "pairingsocketclient.hpp"

namespace MicaPairingService::Network
{
    bool PairingSocketClient::SendPairingConfirmation(const std::string &name, const std::string &ip, const uint16_t port)
    {
        const std::string message = "PAIR\n" + name + "\n" + ip + "\n" + std::to_string(port) + "\n";
        return SendMessage(message);
    }

    bool PairingSocketClient::SendPairingCancellation()
    {
        return SendMessage("CANCEL\n");
    }

    std::string PairingSocketClient::RequestPin(const std::string &name, const std::string &ip, const uint16_t port)
    {
        int clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (clientFd < 0) return "";

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);

        if (connect(clientFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        {
            close(clientFd);
            return "";
        }

        const std::string message = "EXCHANGE_CODE\n" + name + "\n" + ip + "\n" + std::to_string(port) + "\n";
        if (write(clientFd, message.c_str(), message.length()) != static_cast<ssize_t>(message.length()))
        {
            close(clientFd);
            return "";
        }

        char buffer[64];
        const ssize_t bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);
        close(clientFd);

        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            return std::string(buffer);
        }
        return "";
    }

    bool PairingSocketClient::SendMessage(const std::string &msg)
    {
        int clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (clientFd < 0)
        {
            // Failed to create Unix socket
            return false;
        }

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);

        if (connect(clientFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        {
            // Failed to connect to socket path
            close(clientFd);
            return false;
        }

        const ssize_t bytesSent = write(clientFd, msg.c_str(), msg.length());
        close(clientFd);

        return bytesSent == static_cast<ssize_t>(msg.length());
    }
}
