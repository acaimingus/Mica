/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Launcher class for MicaPairingService handling device selection and pairing flow.
 */

#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "../Network/pairingsocketclient.hpp"
#include "../Network/receiveddevice.hpp"
#include "../Terminal/terminallauncher.hpp"
#include "../Tui/deviceselectiontui.hpp"
#include "../Tui/pairingconfirmationtui.hpp"

namespace MicaPairingService::Lifecycle
{
/// @brief Main launcher class for the MicaPairingService
class Launcher
{
  public:
    /// @brief Handles device selection either automatically or via TUI
    /// @param argc Command-line argument count
    /// @param argv Command-line argument strings
    static void HandleDeviceSelection(int argc, char *argv[]);

  private:
    /// @brief Parses device configurations from command-line arguments (passed as triplets of Name, IP, Port)
    /// @param argc Command-line argument count
    /// @param argv Command-line argument strings
    /// @return Vector of parsed ReceivedDevice objects
    static std::vector<Network::ReceivedDevice> ParseCommandLineArgs(int argc, char *argv[]);
};
} // namespace MicaPairingService::Lifecycle
