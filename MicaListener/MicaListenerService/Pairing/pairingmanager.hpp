/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Notification manager class for handling desktop notifications via libnotify.
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "../Network/deviceregistry.hpp"
#include "../Network/networkconfig.hpp"

namespace MicaListener::MicaListenerService::Pairing
{
    /// @brief Encapsulates libnotify initialization, background GLib loop, and notification actions
    class PairingManager
    {
    public:
        /// @brief Initializes libnotify and starts the background GLib event loop for notification callbacks
        static void Initialize();

        /// @brief Method for handling the pairing workflow from start to finish
        /// @param newDeviceConfig Network configuration of the device requesting to pair
        /// @param deviceRegistry Device registry containing all found devices until now
        /// @return the network configuration of the device to pair if accepted, else it's empty
        static std::optional<Network::NetworkConfig> HandleDevicePairing(
            const Network::NetworkConfig &newDeviceConfig, const Network::DeviceRegistry &deviceRegistry);

    private:
        /// @brief Name of this class for the logger
        static constexpr std::string_view logName = "\033[35mNOTIFY\033[0m\t\t";

        /// @brief Displays a desktop notification for a newly discovered Mica device and blocks until the user responds or dismisses it
        /// @param config The network configuration of the discovered device
        /// @return true if the user clicked the notification to pair, false if dismissed or ignored
        static bool RequestDesktopNotification(const Network::NetworkConfig &config);

        /// @brief Helper method for getting the path of the MicaPairing service
        /// @return the path of the MicaPairing executable
        static std::string GetExecutableDir();
    };
}
