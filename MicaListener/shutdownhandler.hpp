/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Handler for a gentle shutdown of the program, where it cleans up its own resources.
 */

#pragma once

#include <atomic>
#include <csignal>
#include <iostream>
#include <ostream>

class ShutdownHandler
{
public:
    /// @brief Method for handling the requested shutdown by the system
    /// @param signal The signal sent by the OS
    static void HandleShutdown(int signal)
    {
        std::clog << logName << "Shutdown requested with the signal " << signal << std::endl;
        shouldShutdown.store(true);
    }

    /// @brief Setup method for the Shutdown Handler
    static void Setup()
    {
        std::signal(SIGINT, HandleShutdown);
        std::signal(SIGTERM, HandleShutdown);
    }

    /// @brief Small helper method for returning the value if the program should shut down
    static bool ShouldShutdown()
    {
        return shouldShutdown.load();
    }

private:
    /// @brief Log prefix for the Shutdown Handler
    static inline const std::string logName = "\033[37mSHUTDOWN\033[0m\t";

    /// @brief Atomic bool for storing if the program should shut down
    static inline std::atomic<bool> shouldShutdown{false};
};
