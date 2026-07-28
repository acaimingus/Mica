/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Main entry point of the program when executed; Sets up the shutdown handler and launches the listener.
 */

#include "MicaListenerService/Lifecycle/shutdownhandler.hpp"
#include "MicaListenerService/Lifecycle/launcher.hpp"

int main()
{
    MicaListener::MicaListenerService::Lifecycle::ShutdownHandler::Setup();
    MicaListener::MicaListenerService::Lifecycle::Launcher::Launch();
}
