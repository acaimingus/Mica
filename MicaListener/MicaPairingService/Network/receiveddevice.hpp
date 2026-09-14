/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Data Transfer Object (DTO) representing a discovered network device.
 *              This class remains header-only because it is essentially a lightweight DTO
 *              with trivial accessors, which enables compiler inlining without compilation unit overhead.
 */

#pragma once

#include <string>
#include <utility>

namespace MicaPairingService::Network
{
/// @brief Lightweight Data Transfer Object (DTO) for discovered network devices
class ReceivedDevice
{
  public:
    /// @brief Constructor for ReceivedDevice
    /// @param _ip IP address of the device
    /// @param _port Port of the device
    /// @param _deviceName Name of the device
    ReceivedDevice(std::string _ip, const int _port, std::string _deviceName)
        : deviceName(std::move(_deviceName)), deviceIp(std::move(_ip)), devicePort(_port)
    {
    }

    /// @brief Public getter for the IP variable
    [[nodiscard]] std::string GetIp() const
    {
        return deviceIp;
    }

    /// @brief Public getter for the port variable
    [[nodiscard]] int GetPort() const
    {
        return devicePort;
    }

    /// @brief Public getter for the device name
    [[nodiscard]] std::string GetDeviceName() const
    {
        return deviceName;
    }

  private:
    std::string deviceName;
    std::string deviceIp;
    int devicePort;
};
} // namespace MicaPairingService::Network
