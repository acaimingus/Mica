/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Thread-safe registry for managing active discovered Mica devices on the network.
 */

#include "deviceregistry.hpp"

#include <iostream>

namespace MicaListener::MicaListenerService::Network
{
    bool DeviceRegistry::AddOrUpdateDevice(const std::string &name, const std::string &ip, uint16_t port)
    {
        std::lock_guard<std::mutex> lock(registryMutex);

        const auto activeDevices = GetActiveDevices();
        std::clog << "=== Aktuelle Geräte in Registry (" << activeDevices.size() << ") ===" << std::endl;
        for (const auto &dev : activeDevices)
        {
            std::clog << "  -> " << dev.GetDeviceName()
                      << " [" << dev.GetIp() << ":" << dev.GetPort() << "]" << std::endl;
        }

        NetworkConfig newConfig(ip, port, name, std::chrono::steady_clock::now());

        auto [it, inserted] = devices.insert_or_assign(name, newConfig);

        if (!inserted)
        {
            // Updated existing device
            return false;
        }

        if (onDeviceAdded)
        {
            onDeviceAdded(newConfig);
        }

        // Added brand new device
        return true;
    }

    bool DeviceRegistry::RemoveDevice(const std::string &name)
    {
        std::lock_guard<std::mutex> lock(registryMutex);
        return devices.erase(name) > 0;
    }

    std::vector<NetworkConfig> DeviceRegistry::GetActiveDevices() const
    {
        std::lock_guard<std::mutex> lock(registryMutex);
        std::vector<NetworkConfig> result;
        result.reserve(devices.size());

        for (const auto &[name, config] : devices)
        {
            result.push_back(config);
        }

        return result;
    }

    void DeviceRegistry::Clear()
    {
        std::lock_guard<std::mutex> lock(registryMutex);
        devices.clear();
    }

    void DeviceRegistry::SetOnDeviceAdded(DeviceAddedCallback callback)
    {
        std::lock_guard<std::mutex> lock(registryMutex);
        onDeviceAdded = std::move(callback);
    }
}
