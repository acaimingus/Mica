#pragma once
#include <string>
#include <utility>

namespace MicaPairingService::Network
{
    class ReceivedDevice
    {
    public:
        ReceivedDevice(std::string _ip, const int _port, std::string _deviceName) : deviceName(std::move(_deviceName)), deviceIp(std::move(_ip)), devicePort(_port)
        {
            // Empty
        };

        /// @brief Public getter for the IP variable
        [[nodiscard]] std::string GetIp() const
        {
            return deviceIp;
        }

        /// @brief Public getter for the port variable
        [[nodiscard]] int GetPort() const
        {
            return devicePort;
        }

        /// @brief Public getter for the device name
        [[nodiscard]] std::string GetDeviceName() const
        {
            return deviceName;
        }

    private:
        std::string deviceName;
        std::string deviceIp;
        int devicePort;
    };
}

