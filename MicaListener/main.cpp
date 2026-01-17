#include <iostream>
#include <string>

#include "servicediscovery.cpp"
#include "socketclient.cpp"
#include "audioplayer.cpp"
#include "sinkmanager.hpp"

namespace MicaListener
{

    class Launcher
    {
    public:
        void Launch()
        {
            std::clog << logName << "MicaListener started..." << std::endl;
            
            std::clog << logName << "Creating sink device..." << std::endl;
            SinkManager sinkManager;
            std::clog << logName << "Sink device created successfully!" << std::endl;

            setenv("PULSE_SINK", sinkManager.sinkName.c_str(), 1);
            std::clog << logName << "Enforced PulseAudio Sink: " << sinkManager.sinkName << std::endl;

            std::clog << logName << "Preparing audio player..." << std::endl;
            audioPlayer.Initialize("");
            std::clog << logName << "Audio player is set up!" << std::endl;

            std::clog << logName << "Listening for services..." << std::endl;
            ListenForService();
        }

    private:
        /// @brief Log prefix for the main launcher
        static inline const std::string logName = "\033[33mMAIN\033[0m\t\t";
        static inline const std::string serviceName = "_micaapp._tcp";
        AudioPlayer audioPlayer;

        void ListenForService()
        {
            // Initialize the Service Discovery
            std::clog << logName << "Creating the Service Discovery for '" << serviceName << "'..." << std::endl;
            MicaListener::ServiceDiscovery serviceDiscovery(serviceName);

            // Set the callback when the Service connection gets lost again
            serviceDiscovery.SetOnServiceLost(
                [this](const std::string &name)
                {
                    std::clog << logName << "Connection loss acknowledged." << std::endl;
                });
            // Set the Callback when the Service gets resolved
            serviceDiscovery.SetOnServiceResolved(
                [this](const MicaListener::NetworkConfig &_config)
                {
                    std::clog << logName << "Received Service info: " << _config.GetIp() << " / " << _config.GetPort() << std::endl;
                    ConnectToService(_config);
                });

            // Find the needed service
            std::clog << logName << "Looking for services..." << std::endl;
            serviceDiscovery.FindService();
        }

        void ConnectToService(const NetworkConfig &_config)
        {
            SocketClient socketClient(_config);

            int buffersize = 4096 * 2;

            std::vector<uint8_t> buffer(buffersize);

            while (true)
            {
                ssize_t bytesRead = socketClient.Read(buffer);
                if (bytesRead <= 0) {
                    // Exit loop on error or connection closed
                    break; 
                }

                std::vector<uint8_t> chunk(buffer.begin(), buffer.begin() + bytesRead);

                audioPlayer.PlayBuffer(chunk);
            }
        }
    };
}

int main()
{
    MicaListener::Launcher launcher;
    launcher.Launch();
}

