/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Main entry point of the pairing service when executed.
 */

#include "Lifecycle/shutdownhandler.hpp"
#include "Lifecycle/launcher.cpp"

int main(const int argc, char* argv[])
{
    MicaPairingService::Lifecycle::ShutdownHandler::Setup();
    MicaPairingService::Lifecycle::Launcher::HandleDeviceSelection(argc, argv);
}