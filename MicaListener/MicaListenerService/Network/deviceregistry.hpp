/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Thread-safe registry for managing active discovered Mica devices on the network.
 */

#pragma once

#include <map>
#include <set>
#include <mutex>
#include <vector>
#include <functional>
#include "networkconfig.hpp"

namespace MicaListener::MicaListenerService::Network
{
    class DeviceRegistry
    {
    public:
        using DeviceAddedCallback = std::function<void(const NetworkConfig &)>;

        DeviceRegistry() = default;

        /// @brief Adds a new device or updates an existing device in the registry
        /// @return true if a brand new device was added, false if an existing device was updated
        bool AddOrUpdateDevice(const std::string &name, const std::string &ip, uint16_t port);

        /// @brief Removes a device from the registry by its service name
        /// @return true if device was removed, false if it was not found
        bool RemoveDevice(const std::string &name);

        /// @brief Retrieves a snapshot list of all currently active device network configurations
        [[nodiscard]] std::vector<NetworkConfig> GetActiveDevices() const;

        /// @brief Clears all registered devices
        void Clear();

        /// @brief Sets a callback to be invoked whenever a brand new device is discovered
        void SetOnDeviceAdded(DeviceAddedCallback callback);

        /// @brief Blacklists a device so it is no longer returned by GetActiveDevices
        void BlacklistDevice(const std::string &name);

    private:
        mutable std::mutex registryMutex;
        std::map<std::string, NetworkConfig> devices;
        std::set<std::string> blacklistedDevices;
        DeviceAddedCallback onDeviceAdded;
    };
}
