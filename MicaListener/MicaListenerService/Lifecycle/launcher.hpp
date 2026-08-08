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

namespace MicaListener::MicaListenerService::Lifecycle
{
    /// @brief Main launcher class that drives the service discovery and audio streaming loop
    class Launcher
    {
    public:
        inline static Network::DeviceRegistry deviceRegistry;

        /// @brief Runs the main program loop: repeatedly discovers the Mica service and
        ///        streams audio until a shutdown is requested
        static void Launch()
        {
            std::clog << logName << "MicaListener started..." << std::endl;

            Pairing::PairingManager::Initialize();

            // Main loop of the program
            while (!ShutdownHandler::ShouldShutdown())
            {
                std::clog << logName << "Creating the Service Discovery for '" << serviceName << "'..." << std::endl;
                Network::ServiceDiscovery serviceDiscovery(serviceName, deviceRegistry);
                std::clog << logName << "Listening for services..." << std::endl;
                Network::NetworkConfig config = serviceDiscovery.ListenForService();

                // If the app should shut down, then break the main loop
                if (ShutdownHandler::ShouldShutdown())
                {
                    break;
                }
                // If the config is invalid, then skip it
                if (config.GetIp().empty())
                {
                    continue;
                }

                // Check if the newly found device is a new device
                const bool isNew = deviceRegistry.AddOrUpdateDevice(config.GetDeviceName(), config.GetIp(),
                                                                    config.GetPort());
                if (isNew)
                {
                    // If it is a new device then handle device pairing
                    const auto selectedConfig = Pairing::PairingManager::HandleDevicePairing(config, deviceRegistry);

                    if (!selectedConfig.has_value())
                    {
                        continue;
                    }

                    Network::Sockets::AndroidSocketClient::ConnectToService(*selectedConfig);
                    // Here the Listener is connected and listening
                    std::clog << logName << "Connection lost or ended." << std::endl;
                }

                // Do a little cooldown after a connection ended to avoid connection storms
                if (!ShutdownHandler::ShouldShutdown())
                {
                    std::clog << logName << "Waiting 3 seconds before retrying..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                }
            }
        }

    private:
        /// @brief Log prefix for the main launcher
        static constexpr std::string logName = "\033[33mMAIN\033[0m\t\t";
        /// @brief The name of the service to look for
        static constexpr std::string serviceName = "_micaapp._tcp";
    };
}
