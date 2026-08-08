/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Handler for gentle shutdown and cancellation signaling for MicaPairingService.
 */

#pragma once

#include <csignal>
#include <iostream>
#include <string>
#include <unistd.h>

#include "../Network/pairingsocketclient.hpp"

namespace MicaPairingService::Lifecycle
{
    class ShutdownHandler
    {
    public:
        /// @brief Method for handling requested shutdown or terminal closure signals
        /// @param signal The signal sent by the OS
        static void HandleShutdown([[maybe_unused]] const int signal)
        {
            Network::PairingSocketClient::SendPairingCancellation();
            _exit(0);
        }

        /// @brief Sets up signal handlers for terminal close (SIGHUP), Ctrl+C (SIGINT), and kill (SIGTERM)
        static void Setup()
        {
            std::signal(SIGHUP, HandleShutdown);
            std::signal(SIGINT, HandleShutdown);
            std::signal(SIGTERM, HandleShutdown);
        }
    };
}
