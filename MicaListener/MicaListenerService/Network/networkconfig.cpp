/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Container class implementation for passing a network configuration containing IP, port, device name and timestamp.
 */

#include "networkconfig.hpp"

namespace MicaListener::MicaListenerService::Network
{
    NetworkConfig::NetworkConfig(std::string _ip, const int _port, std::string _deviceName, const TimePoint _lastSeen)
        : ip(std::move(_ip)), port(_port), deviceName(std::move(_deviceName)), lastSeen(_lastSeen)
    {
    }

    std::string NetworkConfig::GetIp() const
    {
        return ip;
    }

    int NetworkConfig::GetPort() const
    {
        return port;
    }

    std::string NetworkConfig::GetDeviceName() const
    {
        return deviceName;
    }

    NetworkConfig::TimePoint NetworkConfig::GetLastSeen() const
    {
        return lastSeen;
    }

    void NetworkConfig::Touch()
    {
        lastSeen = std::chrono::steady_clock::now();
    }

    std::vector<uint8_t> NetworkConfig::GetSharedSecret() const
    {
        return sharedSecret;
    }

    void NetworkConfig::SetSharedSecret(const std::vector<uint8_t> &secret)
    {
        sharedSecret = secret;
    }
}
