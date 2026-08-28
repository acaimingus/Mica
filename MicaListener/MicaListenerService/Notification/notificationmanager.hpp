#pragma once

#include <string_view>
#include <string>
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <thread>
#include <glib.h>
#include <libnotify/notify.h>

#include "../Network/networkconfig.hpp"
#include "../Lifecycle/shutdownhandler.hpp"

namespace MicaListener::MicaListenerService::Notification
{
    /// @brief Encapsulates libnotify initialization, background GLib loop, and notification actions
    class NotificationManager
    {
    public:
        /// @brief Initializes libnotify and starts the background GLib event loop for notification callbacks
        static void Initialize();

        /// @brief Displays a desktop notification for a newly discovered Mica device and blocks until the user responds or dismisses it
        /// @param config The network configuration of the discovered device
        /// @return true if the user clicked the notification to pair, false if dismissed or ignored
        static bool RequestDesktopNotification(const Network::NetworkConfig &config);

    private:
        /// @brief Name of this class for the logger
        static constexpr std::string_view logName = "\033[36mNOTIFY\033[0m\t\t";
    };
}