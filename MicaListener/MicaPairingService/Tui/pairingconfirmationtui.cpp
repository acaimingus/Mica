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

            std::cout << "Starting secure exchange with " << deviceToPair.GetDeviceName() << "..." << std::endl;

            const std::string pin = Network::PairingSocketClient::RequestPin(deviceToPair.GetDeviceName(), deviceToPair.GetIp(), deviceToPair.GetPort());
            if (pin.empty())
            {
                std::cout << "Failed to generate pairing code." << std::endl;
                Network::PairingSocketClient::SendPairingCancellation();
                return;
            }

            std::cout << deviceToPair.GetDeviceName() << " with the IP " << deviceToPair.GetIp() << ":" << deviceToPair.GetPort() << " would like to pair." << std::endl;
            std::cout << "The pairing code is: " << pin << ". Would you like to pair? [y/n]" << std::endl;
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
