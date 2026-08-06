#include <vector>

#include "../Network/receiveddevice.hpp"
#include "../Network/pairingsocketclient.hpp"
#include "../Tui/pairingconfirmationtui.cpp"
#include "../Tui/deviceselectiontui.cpp"
#include "../Terminal/terminallauncher.hpp"

namespace MicaPairingService::Lifecycle
{
    class Launcher
    {
    public:
        static void HandleDeviceSelection(const int argc, char* argv[])
        {
            /* TODO: Determine if a GUI can be displayed, if not then use TUI as a back up
             * If all fails, show a console message to manually open the selection
             */

            // Try launching a terminal
            Terminal::TerminalLauncher::EnsureTerminalWindow(argc, argv);

            std::cout << "=== MicaPairingService TUI ===" << std::endl;

            const std::vector<Network::ReceivedDevice> devices = ParseCommandLineArgs(argc, argv);

            if (devices.empty())
            {
                std::cout << "No active devices available for pairing." << std::endl;
                Network::PairingSocketClient::SendPairingCancellation();
                return;
            }

            if (devices.size() > 1)
            {
                // There's more than one device asking to pair, open selection
                Tui::DeviceSelectionTui::ShowDeviceSelectionTui(devices);
            }
            else
            {
                // There's only one device, open pairing confirmation
                Tui::PairingConfirmationTui::ShowPairingConfirmationTui(devices.front());
            }
        }

    private:
        static std::vector<Network::ReceivedDevice> ParseCommandLineArgs(const int argc, char *argv[])
        {
            std::vector<Network::ReceivedDevice> devices;

            // Remove the process name
            const int numArgs = argc - 1;

            // Ensure arguments are given in triplets
            if (numArgs == 0 || numArgs % 3 != 0)
            {
                std::cerr << "Invalid arguments: Expected arguments in triplets of (Name, IP, Port)." << std::endl;
                return devices;
            }

            // Jump through the arguments and get the configs
            for (int i = 1; i < argc; i += 3)
            {
                std::string name = argv[i];
                std::string ip = argv[i + 1];
                int port = std::stoi(argv[i + 2]);
                devices.emplace_back(ip, port, name);
            }
            return devices;
        }
    };
}
