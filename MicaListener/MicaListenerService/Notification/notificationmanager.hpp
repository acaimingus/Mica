/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Notification manager class for handling desktop notifications via libnotify.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <future>
#include <glib.h>
#include <iostream>
#include <libnotify/notify.h>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include "../Lifecycle/shutdownhandler.hpp"
#include "../Network/networkconfig.hpp"

namespace MicaListener::MicaListenerService::Notification
{
/// @brief Encapsulates libnotify initialization, background GLib loop, and notification actions
class NotificationManager
{
  public:
    /// @brief Initializes libnotify and starts the background GLib event loop for notification callbacks
    static void Initialize();

    /// @brief Displays a desktop notification for a newly discovered Mica device and blocks until the user responds or
    /// dismisses it
    /// @param config The network configuration of the discovered device
    /// @return true if the user clicked the notification to pair, false if dismissed or ignored
    static bool RequestDesktopPairingNotification(const Network::NetworkConfig &config);

    /// @brief Displays a desktop notification to notify the user that the connection between the phone and the computer
    /// was successful
    static void RequestDesktopConnectedNotification();

  private:
    /// @brief Name of this class for the logger
    static constexpr std::string_view logName = "\033[36mNOTIFY\033[0m\t\t";
};
} // namespace MicaListener::MicaListenerService::Notification