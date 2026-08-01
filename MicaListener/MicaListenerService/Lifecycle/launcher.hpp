/*
* Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Launcher class for setting up the listener service and starting it.
 */
#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>
#include <glib.h>
#include <libnotify/notify.h>
#include <future>
#include <atomic>
#include <memory>
#include <filesystem>
#include <climits>

#include "shutdownhandler.hpp"
#include "../Network/servicediscovery.hpp"
#include "../Network/socketclient.hpp"
#include "../Audio/audioplayer.hpp"
#include "../Audio/sinkmanager.hpp"
#include "../Network/deviceregistry.hpp"

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

            notify_init("MicaListener");
            GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
            std::thread glibThread([loop]() {
                g_main_loop_run(loop);
            });
            glibThread.detach();

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

                bool isNew = deviceRegistry.AddOrUpdateDevice(config.GetDeviceName(), config.GetIp(), config.GetPort());
                bool doPair = false;

                if (isNew)
                {
                    struct SyncData {
                        std::promise<bool> prom;
                        std::atomic<bool> handled{false};
                    };
                    auto syncData = std::make_shared<SyncData>();
                    auto* userDataAction = new std::shared_ptr<SyncData>(syncData);
                    auto* userDataClosed = new std::shared_ptr<SyncData>(syncData);

                    NotifyNotification *n = notify_notification_new("New Mica Device Found", config.GetDeviceName().c_str(), "dialog-information");
                    
                    notify_notification_add_action(n, "pair", "Pair Device", [](NotifyNotification *, char *, gpointer user_data) {
                        auto* s = static_cast<std::shared_ptr<SyncData>*>(user_data);
                        if (!(*s)->handled.exchange(true)) {
                            (*s)->prom.set_value(true);
                        }
                    }, userDataAction, [](gpointer user_data) {
                        delete static_cast<std::shared_ptr<SyncData>*>(user_data);
                    });

                    g_signal_connect_data(n, "closed", G_CALLBACK(+[](NotifyNotification *, gpointer user_data) {
                        auto* s = static_cast<std::shared_ptr<SyncData>*>(user_data);
                        if (!(*s)->handled.exchange(true)) {
                            (*s)->prom.set_value(false);
                        }
                    }), userDataClosed, [](gpointer user_data, GClosure*) {
                        delete static_cast<std::shared_ptr<SyncData>*>(user_data);
                    }, static_cast<GConnectFlags>(0));

                    GError *error = nullptr;
                    if (notify_notification_show(n, &error)) {
                        std::clog << logName << "Waiting for user decision on notification..." << std::endl;
                        doPair = syncData->prom.get_future().get();
                    } else {
                        std::cerr << logName << "Failed to show notification: " << error->message << std::endl;
                        g_error_free(error);
                    }

                    g_object_unref(n);

                    if (doPair) {
                        std::clog << logName << "User selected Pair. Launching MicaPairingService..." << std::endl;
                        std::string exePath = GetExecutableDir() + "/MicaPairingService";
                        
                        std::vector<std::string> args = {exePath};
                        auto activeDevices = deviceRegistry.GetActiveDevices();
                        for (const auto& dev : activeDevices) {
                            args.push_back(dev.GetDeviceName() + "," + dev.GetIp() + "," + std::to_string(dev.GetPort()));
                        }
                        
                        std::vector<char*> cArgs;
                        for (auto& a : args) cArgs.push_back(a.data());
                        cArgs.push_back(nullptr);
                        
                        GError *spawnError = nullptr;
                        if (!g_spawn_async(nullptr, cArgs.data(), nullptr, G_SPAWN_DEFAULT, nullptr, nullptr, nullptr, &spawnError)) {
                            std::cerr << logName << "Failed to spawn MicaPairingService: " << spawnError->message << std::endl;
                            g_error_free(spawnError);
                        }
                    } else {
                        std::clog << logName << "User ignored notification." << std::endl;
                    }
                }

                ConnectToService(config);
                // Here the Listener is connected and listening
                std::clog << logName << "Connection lost or ended." << std::endl;

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
                    std::clog << logName << "Connection loss  with " << name << " acknowledged." << std::endl;
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

        static std::string GetExecutableDir() {
            char result[PATH_MAX];
            ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
            if (count != -1) {
                std::filesystem::path p(std::string(result, count));
                return p.parent_path().string();
            }
            return ".";
        }
    };
}