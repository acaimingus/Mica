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
        inline static Network::DeviceRegistry deviceRegistry;

        /// @brief Runs the main program loop: repeatedly discovers the Mica service and
        ///        streams audio until a shutdown is requested
        static void Launch()
        {
            std::clog << logName << "MicaListener started..." << std::endl;

            Notification::NotificationManager::Initialize();

            std::clog << logName << "Creating the Service Discovery for '" << serviceName << "'..." << std::endl;
            Network::ServiceDiscovery serviceDiscovery(serviceName, deviceRegistry);

            std::atomic<bool> initialBatchDone{false};
            serviceDiscovery.SetOnBatchComplete(
                [&]()
                {
                    std::clog << logName << "Avahi initial batch scan completed." << std::endl;
                    initialBatchDone = true;
                });

            std::clog << logName << "Starting background service discovery..." << std::endl;
            serviceDiscovery.Start();

            // Wait for initial Avahi batch or shutdown
            while (!ShutdownHandler::ShouldShutdown() && !initialBatchDone)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            // Main loop of the program
            while (!ShutdownHandler::ShouldShutdown())
            {
                const auto activeDevices = deviceRegistry.GetActiveDevices();
                if (activeDevices.empty())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    continue;
                }

                const auto &config = activeDevices.front();
                const auto selectedConfig = Pairing::PairingManager::HandleDevicePairing(config, deviceRegistry);

                if (!selectedConfig.has_value())
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }

                Network::Sockets::AndroidSocketClient::ConnectToService(*selectedConfig);
                std::clog << logName << "Connection lost or ended." << std::endl;

                if (!ShutdownHandler::ShouldShutdown())
                {
                    std::clog << logName << "Waiting 3 seconds before retrying..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                }
            }

            serviceDiscovery.Stop();
        }

    private:
        /// @brief Log prefix for the main launcher
        static constexpr std::string logName = "\033[33mMAIN\033[0m\t\t";
        /// @brief The name of the service to look for
        static constexpr std::string serviceName = "_micaapp._tcp";
    };
}
