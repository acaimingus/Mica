/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Implementation of ShutdownHandler for gentle shutdown and cancellation in MicaPairingService.
 */

#include "shutdownhandler.hpp"

namespace MicaPairingService::Lifecycle
{
    void ShutdownHandler::HandleShutdown([[maybe_unused]] const int signal)
    {
        Network::PairingSocketClient::SendPairingCancellation();
        _exit(0);
    }

    void ShutdownHandler::Setup()
    {
        std::signal(SIGHUP, HandleShutdown);
        std::signal(SIGINT, HandleShutdown);
        std::signal(SIGTERM, HandleShutdown);
    }
}
