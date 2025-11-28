#include <iostream>
#include <string>

#include "servicediscovery.cpp"
#include "socketclient.cpp"

namespace MicaListener
{

    class Launcher
    {
    public:
        void Launch()
        {
            std::clog << logMain << "MicaListener started..." << std::endl;
            ListenForService();
        }

    private:
        /// @brief Log prefix for the main launcher
        const std::string logMain = "\033[33mMAIN\033[0m\t\t";
        const std::string serviceName = "_micaapp._tcp";

        void ListenForService()
        {
            // Initialize the Service Discovery
            std::clog << logMain << "Creating the Service Discovery for '" << serviceName << "'..." << std::endl;
            MicaListener::ServiceDiscovery serviceDiscovery(serviceName);

            // Set the callback when the Service connection gets lost again
            serviceDiscovery.SetOnServiceLost(
                [this](const std::string &name)
                {
                    std::clog << logMain << "Connection loss acknowledged." << std::endl;
                });
            // Set the Callback when the Service gets resolved
            serviceDiscovery.SetOnServiceResolved(
                [this](const MicaListener::NetworkConfig &_config)
                {
                    std::clog << logMain << "Received Service info: " << _config.GetIp() << " / " << _config.GetPort() << std::endl;
                    ConnectToService(_config);
                });

            // Find the needed service
            std::clog << logMain << "Looking for services..." << std::endl;
            serviceDiscovery.FindService();
        }

        void ConnectToService(NetworkConfig config)
        {
            SocketClient socketClient(config);

            int buffersize = 4096 * 2;

            std::vector<uint8_t> buffer(buffersize);

            while (true)
            {
                ssize_t bytesRead = socketClient.Read(buffer);
                if (bytesRead <= 0) {
                    // Exit loop on error or connection closed
                    break; 
                }

                std::cout << "Read " << bytesRead << " bytes from the service." << std::endl;
            }
        }
    };
}

int main()
{
    MicaListener::Launcher launcher;
    launcher.Launch();
}

