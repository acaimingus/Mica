#pragma once

#include <iostream>
#include <ostream>

#include "../Network/pairingsocketclient.hpp"
#include "../Network/receiveddevice.hpp"

namespace MicaPairingService::Tui
{
    class PairingConfirmationTui
    {
    public:
        void ShowPairingConfirmationTui(const Network::ReceivedDevice &deviceToPair)
        {
            std::cout << deviceToPair.GetDeviceName() << " with the IP " << deviceToPair.GetIp() << ":" << deviceToPair.GetPort() << " would like to pair." << std::endl;
            std::cout << "Would you like to pair this device? [y/n]" << std::endl;
            std::cin >> userChoice;
            std::cout << std::endl;

            if (userChoice == "y" || userChoice == "Y")
            {
                Network::PairingSocketClient::SendPairingConfirmation(deviceToPair.GetDeviceName(), deviceToPair.GetIp(), deviceToPair.GetPort());
            }
            else
            {
                Network::PairingSocketClient::SendPairingCancellation();
            }
        }
    private:
        std::string userChoice;
    };
}
