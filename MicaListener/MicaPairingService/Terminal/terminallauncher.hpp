/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Terminal launcher helper to ensure MicaPairingService runs inside a visible terminal emulator.
 */

#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>

namespace MicaPairingService::Terminal
{
    /// @brief Helper class to ensure MicaPairingService runs inside an interactive, visible terminal window
    class TerminalLauncher
    {
    public:
        /// @brief Ensures the process is running in an interactive TTY window.
        ///        If not, spawns an available terminal emulator running this executable and exits the background parent.
        /// @param argc Command line argument count
        /// @param argv Command line argument values
        static void EnsureTerminalWindow(int argc, char *argv[]);
    };
}
