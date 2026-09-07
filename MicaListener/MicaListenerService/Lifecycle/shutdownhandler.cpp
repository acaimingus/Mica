/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Implementation of ShutdownHandler for gentle shutdown of the listener service.
 */

#include "shutdownhandler.hpp"

namespace MicaListener::MicaListenerService::Lifecycle
{
    void ShutdownHandler::HandleShutdown(int signal)
    {
        std::clog << logName << "Shutdown requested with the signal " << signal << std::endl;
        shouldShutdown.store(true);
    }

    void ShutdownHandler::Setup()
    {
        std::signal(SIGINT, HandleShutdown);
        std::signal(SIGTERM, HandleShutdown);
    }

    bool ShutdownHandler::ShouldShutdown()
    {
        return shouldShutdown.load();
    }
}
