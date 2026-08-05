#pragma once

#include <iostream>
#include <ostream>

#include "../Network/pairingsocketclient.hpp"

namespace MicaPairingService::Tui
{
    class PairingConfirmationTui
    {
    public:
        void ShowPairingConfirmationTui()
        {
            std::cout << "Would you like to pair this device? [y/n]" << std::endl;
            std::cin >> userChoice;
            std::cout << std::endl;

            if (userChoice == "y" || userChoice == "Y")
            {
                Network::PairingSocketClient::SendPairingConfirmation("debug", "127.0.0.1", 42000);
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
