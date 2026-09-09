/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Launcher implementation for MicaPairingService handling device selection and pairing flow.
 */

#include "launcher.hpp"

namespace MicaPairingService::Lifecycle
{
void Launcher::HandleDeviceSelection(const int argc, char *argv[])
{
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

std::vector<Network::ReceivedDevice> Launcher::ParseCommandLineArgs(const int argc, char *argv[])
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
} // namespace MicaPairingService::Lifecycle
