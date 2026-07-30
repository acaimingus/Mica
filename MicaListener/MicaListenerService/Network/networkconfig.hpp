/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Container class for passing a network configuration containing IP, port, device name and timestamp.
 */

#pragma once

#include <string>
#include <utility>
#include <chrono>

namespace MicaListener::MicaListenerService::Network
{
    /// @brief Class for storing the information of a network configuration of a service
    class NetworkConfig
    {
    public:
        using TimePoint = std::chrono::steady_clock::time_point;

        /// @brief Constructor for the network configuration requiring all explicit parameters
        /// @param _ip IP to be used
        /// @param _port Port to be used
        /// @param _deviceName Name of the device
        /// @param _lastSeen Timestamp of the last connection attempt or discovery of the device
        NetworkConfig(std::string _ip, const int _port, std::string _deviceName, const TimePoint _lastSeen)
            : ip(std::move(_ip)), port(_port), deviceName(std::move(_deviceName)), lastSeen(_lastSeen)
        {
        }

        /// @brief Public getter for the IP variable
        [[nodiscard]] std::string GetIp() const
        {
            return ip;
        }

        /// @brief Public getter for the port variable
        [[nodiscard]] int GetPort() const
        {
            return port;
        }

        /// @brief Public getter for the device name
        [[nodiscard]] std::string GetDeviceName() const
        {
            return deviceName;
        }

        /// @brief Public getter for the last seen timestamp
        [[nodiscard]] TimePoint GetLastSeen() const
        {
            return lastSeen;
        }

        /// @brief Updates the lastSeen timestamp to current time
        void Touch()
        {
            lastSeen = std::chrono::steady_clock::now();
        }

    private:
        /// @brief IP of the network configuration
        std::string ip;

        /// @brief Port of the network configuration
        int port;

        /// @brief Name of the device behind the network configuration
        std::string deviceName;

        /// @brief Timestamp of when the device was last seen
        TimePoint lastSeen;
    };
}
