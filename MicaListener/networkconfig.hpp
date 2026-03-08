/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Container class for passing a network configuration containing the IP and a port.
 */

#pragma once

#include <string>
#include <utility>

namespace MicaListener
{
    /// @brief Class for storing the information of a network configuration of a service
    class NetworkConfig
    {
    public:
        /// @brief Constructor for the network configuration
        /// @param _ip IP to be used
        /// @param _port Port to be used
        NetworkConfig(std::string _ip, const int _port) : ip(std::move(_ip)), port(_port)
        {
            // Empty
        }

        /// @brief Delete assignments
        NetworkConfig &operator=(const NetworkConfig &) = delete;

        /// @brief Public getter for the IP variable
        /// @return IP of the network configuration
        [[nodiscard]] std::string GetIp() const
        {
            return ip;
        }

        /// @brief Public getter for the port variable
        /// @return Port of the network configuration
        [[nodiscard]] int GetPort() const
        {
            return port;
        }

    private:
        /// @brief IP of the network configuration
        const std::string ip;

        /// @brief Port of the network configuration
        const int port;
    };
}
