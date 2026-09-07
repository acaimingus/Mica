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
