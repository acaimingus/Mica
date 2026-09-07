#include "launcher.hpp"

namespace MicaListener::MicaListenerService::Lifecycle
{
    void Launcher::Launch()
    {
        std::clog << logName << "mica-listener started..." << std::endl;

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
                std::clog << logName << "Device '" << config.GetDeviceName() <<
                        "' rejected or timed out. Blacklisting..." << std::endl;
                deviceRegistry.BlacklistDevice(config.GetDeviceName());
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
}
