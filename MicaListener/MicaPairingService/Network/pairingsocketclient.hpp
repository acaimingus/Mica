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
    /// @brief Unix Domain Socket client for communicating pairing decisions and requests back to MicaListener
    class PairingSocketClient
    {
    public:
        /// @brief Socket path for IPC communication with MicaListener
        static constexpr auto socketPath = "/tmp/mica_pairing.sock";

        /// @brief Sends a confirmed device choice to MicaListener via Unix Domain Socket
        /// @param name Name of the device
        /// @param ip IP address of the device
        /// @param port Port of the device
        /// @return True if message was sent successfully, false otherwise
        static bool SendPairingConfirmation(const std::string &name, const std::string &ip, uint16_t port);

        /// @brief Sends a cancellation message to MicaListener via Unix Domain Socket
        /// @return True if message was sent successfully, false otherwise
        static bool SendPairingCancellation();

        /// @brief Requests a PIN for a specific device from MicaListener
        /// @param name Name of the device
        /// @param ip IP address of the device
        /// @param port Port of the device
        /// @return Verification PIN string, or empty string on failure
        static std::string RequestPin(const std::string &name, const std::string &ip, uint16_t port);

    private:
        /// @brief Helper method to connect to the Unix socket and send a message
        /// @param msg The raw string message to send
        /// @return True if sent successfully, false otherwise
        static bool SendMessage(const std::string &msg);
    };

}
