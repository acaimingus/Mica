/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Implementation of NotificationManager using libnotify and GLib.
 */

#include "notificationmanager.hpp"

#include <atomic>
#include <future>
#include <iostream>
#include <memory>
#include <thread>
#include <glib.h>
#include <libnotify/notify.h>

namespace MicaListener::MicaListenerService::Notification
{
    void NotificationManager::Initialize()
    {
        std::clog << logName << "Initializing libnotify..." << std::endl;
        notify_init("MicaListener");

        GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
        std::thread glibThread([loop]() {
            g_main_loop_run(loop);
        });
        glibThread.detach();
    }

    bool NotificationManager::RequestPairingConfirmation(const Network::NetworkConfig &config)
    {
        struct SyncData {
            std::promise<bool> prom;
            std::atomic<bool> handled{false};
        };

        auto syncData = std::make_shared<SyncData>();
        auto *userDataClosed = new std::shared_ptr<SyncData>(syncData);

        NotifyNotification *n = notify_notification_new(
            "Mica: New devices found",
            "Click the notification to select the device and confirm pairing",
            "dialog-information"
        );

        auto actionCb = [](NotifyNotification *, char *, gpointer user_data) {
            if (const auto *s = static_cast<std::shared_ptr<SyncData>*>(user_data); !(*s)->handled.exchange(true)) {
                (*s)->prom.set_value(true);
            }
        };

        auto actionFree = [](gpointer user_data) {
            delete static_cast<std::shared_ptr<SyncData>*>(user_data);
        };

        auto *userDataDefault = new std::shared_ptr<SyncData>(syncData);
        notify_notification_add_action(n, "default", "Select and pair device", actionCb, userDataDefault, actionFree);

        g_signal_connect_data(n, "closed", G_CALLBACK(+[](NotifyNotification *, gpointer user_data) {
            const auto *s = static_cast<std::shared_ptr<SyncData>*>(user_data);
            if (!(*s)->handled.exchange(true)) {
                (*s)->prom.set_value(false);
            }
        }), userDataClosed, [](gpointer user_data, GClosure*) {
            delete static_cast<std::shared_ptr<SyncData>*>(user_data);
        }, static_cast<GConnectFlags>(0));

        bool userAccepted = false;
        GError *error = nullptr;

        if (notify_notification_show(n, &error)) {
            std::clog << logName << "Waiting for user decision on notification for '" << config.GetDeviceName() << "'..." << std::endl;
            userAccepted = syncData->prom.get_future().get();
        } else {
            std::cerr << logName << "Failed to show notification: " << (error ? error->message : "unknown") << std::endl;
            if (error) g_error_free(error);
        }

        g_object_unref(n);
        return userAccepted;
    }
}
