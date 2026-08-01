#include <iostream>

namespace MicaPairingService::Tui
{
    class DeviceSelectionTui
    {
        public:
        void ShowDeviceSelectionTui()
        {
            // Inform the user that multiple devices are available and print them out
            std::cout << "Multiple Mica devices have been found. Please choose one:" << std::endl;
            PrintListOfDevices();

            // Get the desired device from the user
            std::cout << "Select device number: ";
            std::cin >> selectedDeviceNumber;
            std::cout << std::endl;
        }

        private:
        int selectedDeviceNumber;

        void PrintListOfDevices()
        {

        }
    };
}
