#include <iostream>
#include <string>

#include "servicediscovery.cpp"
#include "socketclient.cpp"
#include "audioplayer.cpp"
#include "sinkmanager.hpp"
#include "shutdownhandler.cpp"

namespace MicaListener
{

    class Launcher
    {
    public:
        void Launch()
        {
            std::clog << logName << "MicaListener started..." << std::endl;
            std::clog << logName << "Creating sink device..." << std::endl;

            const SinkManager sinkManager;

            std::clog << logName << "Sink device created successfully!" << std::endl;

            setenv("PULSE_SINK", sinkManager.sinkName.c_str(), 1);

            std::clog << logName << "Enforced PulseAudio Sink: " << sinkManager.sinkName << std::endl;
            std::clog << logName << "Preparing audio player..." << std::endl;

            audioPlayer.Initialize("");

            std::clog << logName << "Audio player is set up!" << std::endl;

            // Main loop of the program
            while (!ShutdownHandler::ShouldShutdown())
            {
                std::clog << logName << "Listening for services..." << std::endl;
                NetworkConfig config = ListenForService();

                if (ShutdownHandler::ShouldShutdown()) break;

                // If the config is invalid, then skip it
                if (config.GetIp().empty())
                {
                    continue;
                }

                ConnectToService(config);

                std::clog << logName << "Connection lost or ended. Retrying..." << std::endl;
            }
        }

    private:
        /// @brief Log prefix for the main launcher
        static inline const std::string logName = "\033[33mMAIN\033[0m\t\t";
        static inline const std::string serviceName = "_micaapp._tcp";
        AudioPlayer audioPlayer;

        static NetworkConfig ListenForService()
        {
            std::string foundIp;
            int foundPort = 0;

            // Initialize the Service Discovery
            std::clog << logName << "Creating the Service Discovery for '" << serviceName << "'..." << std::endl;
            ServiceDiscovery serviceDiscovery(serviceName);

            // Set the callback when the Service connection gets lost again
            serviceDiscovery.SetOnServiceLost(
                [&](const std::string &name)
                {
                    std::clog << logName << "Connection loss acknowledged." << std::endl;
                });

            // Set the Callback when the Service gets resolved
            serviceDiscovery.SetOnServiceResolved(
                [&](const NetworkConfig &_config)
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

        void ConnectToService(const NetworkConfig &_config)
        {
            try
            {
                const SocketClient socketClient(_config);
                constexpr int bufferSize = 4096 * 2;
                std::vector<uint8_t> buffer(bufferSize);

                while (!ShutdownHandler::ShouldShutdown())
                {
                    const ssize_t bytesRead = socketClient.Read(buffer);
                    if (bytesRead <= 0) {
                        // Check if there is a timeout
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            continue;
                        }
                        // Exit loop on error or connection closed
                        break;
                    }

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

int main()
{
    ShutdownHandler::Setup();
    MicaListener::Launcher launcher;
    launcher.Launch();
}

