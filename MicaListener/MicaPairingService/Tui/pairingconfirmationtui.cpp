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
        static void ShowPairingConfirmationTui(const Network::ReceivedDevice &deviceToPair)
        {
            std::string choice;

            std::cout << deviceToPair.GetDeviceName() << " with the IP " << deviceToPair.GetIp() << ":" << deviceToPair.GetPort() << " would like to pair." << std::endl;
            std::cout << "Would you like to pair this device? [y/n]" << std::endl;
            std::cin >> choice;
            std::cout << std::endl;

            if (choice == "y" || choice == "Y")
            {
                Network::PairingSocketClient::SendPairingConfirmation(deviceToPair.GetDeviceName(), deviceToPair.GetIp(), deviceToPair.GetPort());
            }
            else
            {
                Network::PairingSocketClient::SendPairingCancellation();
            }
        }
    };
}
