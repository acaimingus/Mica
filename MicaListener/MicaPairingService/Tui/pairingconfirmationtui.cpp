/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Implementation of TUI component for confirming pairing with a single device.
 */

#include "pairingconfirmationtui.hpp"

namespace MicaPairingService::Tui
{
void PairingConfirmationTui::ShowPairingConfirmationTui(const Network::ReceivedDevice &deviceToPair)
{
    std::string choice;

    std::cout << "Starting secure exchange with " << deviceToPair.GetDeviceName() << "..." << std::endl;

    const std::string pin = Network::PairingSocketClient::RequestPin(deviceToPair.GetDeviceName(), deviceToPair.GetIp(),
                                                                     deviceToPair.GetPort());
    if (pin.empty())
    {
        std::cout << "Failed to generate pairing code." << std::endl;
        Network::PairingSocketClient::SendPairingCancellation();
        return;
    }

    std::cout << deviceToPair.GetDeviceName() << " with the IP " << deviceToPair.GetIp() << ":"
              << deviceToPair.GetPort() << " would like to pair." << std::endl;
    std::cout << "The pairing code is: " << pin << ". Would you like to pair? [y/n]" << std::endl;
    std::cin >> choice;
    std::cout << std::endl;

    if (choice == "y" || choice == "Y")
    {
        Network::PairingSocketClient::SendPairingConfirmation(deviceToPair.GetDeviceName(), deviceToPair.GetIp(),
                                                              deviceToPair.GetPort());
    }
    else
    {
        Network::PairingSocketClient::SendPairingCancellation();
    }
}
} // namespace MicaPairingService::Tui
