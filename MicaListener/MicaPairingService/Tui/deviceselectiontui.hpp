/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: TUI component for selecting one device from a list of discovered devices.
 */

#pragma once

#include <iostream>
#include <limits>
#include <vector>

#include "../Network/receiveddevice.hpp"
#include "pairingconfirmationtui.hpp"

namespace MicaPairingService::Tui
{
/// @brief Terminal UI component for displaying multiple discovered devices and letting the user select one
class DeviceSelectionTui
{
  public:
    /// @brief Prompts user to select a device from a list and triggers confirmation for that device
    /// @param devices Vector of discovered devices
    static void ShowDeviceSelectionTui(const std::vector<Network::ReceivedDevice> &devices);

  private:
    /// @brief Prints formatted numbered list of discovered devices to stdout
    /// @param devices Vector of discovered devices
    static void PrintListOfDevices(const std::vector<Network::ReceivedDevice> &devices);
};
} // namespace MicaPairingService::Tui
