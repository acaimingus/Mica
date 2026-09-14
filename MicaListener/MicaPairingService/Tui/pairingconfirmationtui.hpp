/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: TUI component for confirming pairing with a single device.
 */

#pragma once

#include <iostream>
#include <ostream>
#include <string>

#include "../Network/pairingsocketclient.hpp"
#include "../Network/receiveddevice.hpp"

namespace MicaPairingService::Tui
{
    /// @brief Terminal UI component for displaying device information and prompting for pairing confirmation
    class PairingConfirmationTui
    {
    public:
        /// @brief Displays the pairing confirmation dialog in the terminal for the given device
        /// @param deviceToPair The received device requesting to be paired
        static void ShowPairingConfirmationTui(const Network::ReceivedDevice &deviceToPair);
    };
}
