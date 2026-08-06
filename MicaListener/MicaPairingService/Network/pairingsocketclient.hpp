/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Unix domain socket client for MicaPairingService to communicate pairing decisions back to MicaListener.
 */

#pragma once

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstdint>

namespace MicaPairingService::Network
{
    class PairingSocketClient
    {
    public:
        static constexpr auto socketPath = "/tmp/mica_pairing.sock";

        /// @brief Sends a confirmed device choice to MicaListener via Unix Domain Socket
        static bool SendPairingConfirmation(const std::string &name, const std::string &ip, uint16_t port)
        {
            const std::string message = "PAIR\n" + name + "\n" + ip + "\n" + std::to_string(port) + "\n";
            return SendMessage(message);
        }

        /// @brief Sends a cancellation message to MicaListener via Unix Domain Socket
        static bool SendPairingCancellation()
        {
            return SendMessage("CANCEL\n");
        }

    private:
        static bool SendMessage(const std::string &msg)
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
    };
}
