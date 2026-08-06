#pragma once

#include <iostream>
#include <limits>
#include <vector>

#include "../Network/receiveddevice.hpp"
#include "pairingconfirmationtui.cpp"

namespace MicaPairingService::Tui
{
    class DeviceSelectionTui
    {
        public:
        static void ShowDeviceSelectionTui(const std::vector<Network::ReceivedDevice>& devices)
        {
            // Inform the user that multiple devices are available and print them out
            std::cout << "Multiple Mica devices have been found. Please choose one:" << std::endl;
            PrintListOfDevices(devices);

            int choice = -1;
            // Get the desired device from the user
            while (true)
            {
                std::cout << "Select device number (1-" << devices.size() << "): ";
                if (std::cin >> choice && choice >= 1 && static_cast<std::size_t>(choice) <= devices.size())
                {
                    // Valid option
                    break;
                }
                // Invalid option
                std::cout << "Invalid selection! Please enter a valid number.\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }

            if (choice != -1)
            {
                // Open the pairing screen for the selected device
                PairingConfirmationTui::ShowPairingConfirmationTui(devices[choice - 1]);
            }
        }

        private:

        static void PrintListOfDevices(const std::vector<Network::ReceivedDevice>& devices)
        {
            for (std::size_t i = 0; i < devices.size(); ++i)
            {
                std::cout << i + 1 << ". " << devices[i].GetDeviceName() << " (" << devices[i].GetIp() << ":" << devices[i].GetPort() << ")" << std::endl;
            }
        }
    };
}
