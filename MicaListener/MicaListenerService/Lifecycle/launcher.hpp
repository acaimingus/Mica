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

#include "shutdownhandler.hpp"
#include "../Network/servicediscovery.hpp"
#include "../Network/socketclient.hpp"
#include "../Audio/audioplayer.hpp"
#include "../Audio/sinkmanager.hpp"

namespace MicaListener::MicaListenerService::Lifecycle
{
    /// @brief Main launcher class that drives the service discovery and audio streaming loop
    class Launcher
    {
    public:
        /// @brief Runs the main program loop: repeatedly discovers the Mica service and
        ///        streams audio until a shutdown is requested
        static void Launch()
        {
            std::clog << logName << "MicaListener started..." << std::endl;

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
                    std::clog << logName << "Received Service info: " << foundIp << " / " << foundPort << std::endl;
                    serviceDiscovery.StopService();
                });

            // Find the needed service
            std::clog << logName << "Looking for services..." << std::endl;
            serviceDiscovery.FindService();

            return {foundIp, foundPort};
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
    };
}