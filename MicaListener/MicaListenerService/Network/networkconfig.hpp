/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Container class for passing a network configuration containing IP, port, device name and timestamp.
 */

#pragma once

#include <chrono>
#include <string>
#include <utility>
#include <vector>

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
    NetworkConfig(std::string _ip, const int _port, std::string _deviceName, const TimePoint _lastSeen);

    /// @brief Public getter for the IP variable
    /// @return IP address string
    [[nodiscard]] std::string GetIp() const;

    /// @brief Public getter for the port variable
    /// @return Port number
    [[nodiscard]] int GetPort() const;

    /// @brief Public getter for the device name
    /// @return Name of the device
    [[nodiscard]] std::string GetDeviceName() const;

    /// @brief Public getter for the last seen timestamp
    /// @return Timestamp of when the device was last seen
    [[nodiscard]] TimePoint GetLastSeen() const;

    /// @brief Updates the lastSeen timestamp to current time
    void Touch();

    /// @brief Public getter for the shared secret
    /// @return Vector of bytes representing the shared secret
    [[nodiscard]] std::vector<uint8_t> GetSharedSecret() const;

    /// @brief Sets the shared secret for this device
    /// @param secret Vector of bytes representing the shared secret
    void SetSharedSecret(const std::vector<uint8_t> &secret);

  private:
    /// @brief IP of the network configuration
    std::string ip;

    /// @brief Port of the network configuration
    int port;

    /// @brief Name of the device behind the network configuration
    std::string deviceName;

    /// @brief Timestamp of when the device was last seen
    TimePoint lastSeen;

    /// @brief Shared secret for the device pairing
    std::vector<uint8_t> sharedSecret;
};
} // namespace MicaListener::MicaListenerService::Network
