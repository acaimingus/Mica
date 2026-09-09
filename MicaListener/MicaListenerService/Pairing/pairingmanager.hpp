/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Pairing manager class for handling device pairing workflows.
 */

#pragma once

#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <climits>
#include <filesystem>
#include <future>
#include <glib.h>
#include <iostream>
#include <memory>
#include <optional>
#include <poll.h>
#include <signal.h>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "../Lifecycle/shutdownhandler.hpp"
#include "../Network/Cryptography/ecdhkeyexchange.hpp"
#include "../Network/Sockets/pairingsocketserver.hpp"
#include "../Network/deviceregistry.hpp"
#include "../Network/networkconfig.hpp"
#include "../Notification/notificationmanager.hpp"

namespace MicaListener::MicaListenerService::Pairing
{
/// @brief Encapsulates pairing logic for Mica listener
class PairingManager
{
  public:
    /// @brief Method for handling the pairing workflow from start to finish
    /// @param newDeviceConfig Network configuration of the device requesting to pair
    /// @param deviceRegistry Device registry containing all found devices until now
    /// @return the network configuration of the device to pair if accepted, else it's empty
    static std::optional<Network::NetworkConfig> HandleDevicePairing(const Network::NetworkConfig &newDeviceConfig,
                                                                     const Network::DeviceRegistry &deviceRegistry);

  private:
    /// @brief Name of this class for the logger
    static constexpr std::string_view logName = "\033[35mPAIRING\033[0m\t\t";

    /// @brief Helper method for getting the path of the MicaPairing service
    /// @return the path of the MicaPairing executable
    static std::string GetExecutableDir();
};
} // namespace MicaListener::MicaListenerService::Pairing
