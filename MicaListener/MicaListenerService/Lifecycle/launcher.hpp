/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Launcher class for setting up the listener service and starting it.
 */
#pragma once

#include <chrono>
#include <climits>
#include <filesystem>
#include <future>
#include <atomic>
#include <memory>
#include <optional>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <glib.h>

#include "shutdownhandler.hpp"
#include "../Network/servicediscovery.hpp"
#include "../Network/socketclient.hpp"
#include "../Audio/audioplayer.hpp"
#include "../Audio/sinkmanager.hpp"
#include "../Network/deviceregistry.hpp"
#include "../Network/unixsocketserver.hpp"
#include "../Notification/notificationmanager.hpp"

namespace MicaListener::MicaListenerService::Lifecycle
{
    /// @brief Main launcher class that drives the service discovery and audio streaming loop
    class Launcher
    {
    public:
        inline static Network::DeviceRegistry deviceRegistry;
        inline static Network::UnixSocketServer socketServer;

        /// @brief Runs the main program loop: repeatedly discovers the Mica service and
        ///        streams audio until a shutdown is requested
        static void Launch()
        {
            std::clog << logName << "MicaListener started..." << std::endl;

            Notification::NotificationManager::Initialize();

            // Main loop of the program
            while (!ShutdownHandler::ShouldShutdown())
            {
                std::clog << logName << "Listening for services..." << std::endl;
                Network::NetworkConfig config = ListenForService();

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
                    const auto selectedConfig = HandleDevicePairing(config);

                    if (!selectedConfig.has_value())
                    {
                        continue;
                    }

                    ConnectToService(*selectedConfig);
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

        /// @brief Waits until the Mica mDNS service is found and returns its network address
        /// @return A NetworkConfig with the resolved IP and port; IP is empty on shutdown
        static Network::NetworkConfig ListenForService()
        {
            std::string foundIp;
            int foundPort = 0;
            std::string foundName;

            // Initialize the Service Discovery
            std::clog << logName << "Creating the Service Discovery for '" << serviceName << "'..." << std::endl;
            Network::ServiceDiscovery serviceDiscovery(serviceName);

            // Set the callback when the Service connection gets lost again
            serviceDiscovery.SetOnServiceLost(
                [&](const std::string &name)
                {
                    std::clog << logName << "Connection loss with " << name << " acknowledged." << std::endl;
                    deviceRegistry.RemoveDevice(name);
                });

            // Set the Callback when the Service gets resolved
            serviceDiscovery.SetOnServiceResolved(
                [&](const Network::NetworkConfig &_config)
                {
                    foundIp = _config.GetIp();
                    foundPort = _config.GetPort();
                    foundName = _config.GetDeviceName();
                    std::clog << logName << "Received Service info: " << foundIp << " / " << foundPort << std::endl;
                    serviceDiscovery.StopService();
                });

            // Find the needed service
            std::clog << logName << "Looking for services..." << std::endl;
            serviceDiscovery.FindService();

            return Network::NetworkConfig(foundIp, foundPort, foundName, std::chrono::steady_clock::now());
        }

        /// @brief Creates a virtual PulseAudio sink, connects to the given service and
        ///        streams received audio data until the connection is lost or shutdown is requested
        /// @param _config The network address and port of the discovered Mica service
        static void ConnectToService(const Network::NetworkConfig &_config)
        {
            try
            {
                std::clog << logName << "Creating sink device..." << std::endl;
                const Audio::SinkManager sinkManager;
                std::clog << logName << "Sink device created successfully!" << std::endl;

                setenv("PULSE_SINK", sinkManager.sinkName.c_str(), 1);
                std::clog << logName << "Enforced PulseAudio Sink: " << sinkManager.sinkName << std::endl;
                std::clog << logName << "Preparing audio player..." << std::endl;
                Audio::AudioPlayer audioPlayer;
                // I don't know why this device is named Mica and I cannot figure it out
                // But it works, so I guess that's fine?
                audioPlayer.Initialize("Mica");
                std::clog << logName << "Audio player is set up!" << std::endl;

                const Network::SocketClient socketClient(_config);
                constexpr int bufferSize = 4096 * 2;
                std::vector<uint8_t> buffer(bufferSize);

                while (!ShutdownHandler::ShouldShutdown())
                {
                    const ssize_t bytesRead = socketClient.Read(buffer);
                    // There is an error or a timeout
                    if (bytesRead < 0)
                    {
                        // Check if there is a timeout
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            continue;
                        }
                        // Exit loop on error
                        break;
                    }
                    // The connection was closed
                    if (bytesRead == 0)
                    {
                        std::clog << logName << "Connection lost." << std::endl;
                        break;
                    }

                    // Data was received
                    std::vector<uint8_t> chunk(buffer.begin(), buffer.begin() + bytesRead);
                    audioPlayer.PlayBuffer(chunk);
                }
            } catch (const std::runtime_error &error)
            {
                std::cerr << logName << "Connection error: " << error.what() << std::endl;
            }
        }

        static std::string GetExecutableDir()
        {
            char result[PATH_MAX];
            ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
            if (count != -1)
            {
                std::filesystem::path p(std::string(result, count));
                return p.parent_path().string();
            }
            return ".";
        }

        /// @brief Handles desktop notification and IPC pairing confirmation for a newly discovered device
        /// @param config The network configuration of the newly discovered service
        /// @return std::optional<Network::NetworkConfig> containing the user-selected device config, or std::nullopt if ignored/cancelled
        static std::optional<Network::NetworkConfig> HandleDevicePairing(const Network::NetworkConfig &config)
        {
            const bool doPair = Notification::NotificationManager::RequestPairingConfirmation(config);

            if (!doPair)
            {
                std::clog << logName << "User ignored notification. Skipping connection." << std::endl;
                return std::nullopt;
            }

            std::clog << logName << "User selected Pair. Executing MicaPairingService..." << std::endl;
            const std::string exePath = GetExecutableDir() + "/MicaPairingService";

            std::vector<std::string> args = {exePath};
            for (auto activeDevices = deviceRegistry.GetActiveDevices(); const auto &dev: activeDevices)
            {
                args.push_back(dev.GetDeviceName() + "," + dev.GetIp() + "," + std::to_string(dev.GetPort()));
            }

            std::vector<char *> cArgs;
            for (auto &a: args) cArgs.push_back(a.data());
            cArgs.push_back(nullptr);

            struct SyncPairing
            {
                std::promise<std::optional<Network::NetworkConfig> > prom;
                std::atomic<bool> handled{false};
            };
            auto syncPairing = std::make_shared<SyncPairing>();

            socketServer.Start(
                [syncPairing](const std::string &name, const std::string &ip, uint16_t port)
                {
                    if (!syncPairing->handled.exchange(true))
                    {
                        syncPairing->prom.set_value(
                            Network::NetworkConfig(ip, port, name, std::chrono::steady_clock::now()));
                    }
                },
                [syncPairing]()
                {
                    if (!syncPairing->handled.exchange(true))
                    {
                        syncPairing->prom.set_value(std::nullopt);
                    }
                }
            );

            GError *spawnError = nullptr;
            if (!g_spawn_async(nullptr, cArgs.data(), nullptr, G_SPAWN_DEFAULT, nullptr, nullptr, nullptr, &spawnError))
            {
                std::cerr << logName << "Failed to spawn MicaPairingService: " << (spawnError
                    ? spawnError->message
                    : "unknown") << std::endl;
                if (spawnError) g_error_free(spawnError);
                socketServer.Stop();
                return std::nullopt;
            }

            std::clog << logName << "Waiting for pairing decision from TUI via Unix Socket..." << std::endl;

            auto future = syncPairing->prom.get_future();
            std::optional<Network::NetworkConfig> selectedConfigOpt = std::nullopt;

            if (future.wait_for(std::chrono::minutes(1)) == std::future_status::ready)
            {
                selectedConfigOpt = future.get();
            } else
            {
                std::cerr << logName << "Timed out waiting for pairing decision." << std::endl;
            }

            socketServer.Stop();

            if (selectedConfigOpt.has_value())
            {
                std::clog << logName << "Device selected via Unix Socket: " << selectedConfigOpt->GetDeviceName() <<
                        std::endl;
            } else
            {
                std::clog << logName << "Pairing cancelled or rejected." << std::endl;
            }

            return selectedConfigOpt;
        }
    };
}
