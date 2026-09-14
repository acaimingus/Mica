/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Launcher class for setting up the listener service and starting it.
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "shutdownhandler.hpp"
#include "../Network/deviceregistry.hpp"
#include "../Network/servicediscovery.hpp"
#include "../Network/Sockets/androidsocketclient.hpp"
#include "../Pairing/pairingmanager.hpp"
#include "../Notification/notificationmanager.hpp"

namespace MicaListener::MicaListenerService::Lifecycle
{
    /// @brief Main launcher class that drives the service discovery and audio streaming loop
    class Launcher
    {
    public:
        /// @brief The device registry for storing found devices from the Avahi service discovery
        inline static Network::DeviceRegistry deviceRegistry;

        /// @brief Runs the main program loop: repeatedly discovers the Mica service and
        ///        streams audio until a shutdown is requested
        static void Launch();

    private:
        /// @brief Log prefix for the main launcher
        static constexpr std::string logName = "\033[33mMAIN\033[0m\t\t";
        /// @brief The name of the service to look for
        static constexpr std::string serviceName = "_micaapp._tcp";
    };
}
