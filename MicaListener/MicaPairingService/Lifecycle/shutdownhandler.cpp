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
