#pragma once

#include <atomic>
#include <csignal>
#include <iostream>
#include <ostream>

class ShutdownHandler
{
public:
    static void HandleShutdown(int signal)
    {
        std::clog << logName << "Shutdown requested" << std::endl;
        shouldShutdown.store(true);
    }

    static void Setup()
    {
        std::signal(SIGINT, HandleShutdown);
        std::signal(SIGTERM, HandleShutdown);
    }

    static bool ShouldShutdown()
    {
        return shouldShutdown.load();
    }

private:
    static inline const std::string logName = "\033[37mSHUTDOWN\033[0m\t";
    static inline std::atomic<bool> shouldShutdown{false};
};
