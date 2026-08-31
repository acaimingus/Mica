#include "notificationmanager.hpp"

namespace MicaListener::MicaListenerService::Notification
{
    void NotificationManager::Initialize()
    {
        std::clog << logName << "Initializing libnotify..." << std::endl;
        notify_init("MicaListener");

        GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
        std::thread glibThread([loop]()
        {
            g_main_loop_run(loop);
        });
        glibThread.detach();
    }

    bool NotificationManager::RequestDesktopPairingNotification(const Network::NetworkConfig &config)
    {
        struct SyncData
        {
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

        auto actionCb = [](NotifyNotification *, char *, gpointer user_data)
        {
            if (const auto *s = static_cast<std::shared_ptr<SyncData> *>(user_data); !(*s)->handled.exchange(true))
            {
                (*s)->prom.set_value(true);
            }
        };

        auto actionFree = [](const gpointer user_data)
        {
            delete static_cast<std::shared_ptr<SyncData> *>(user_data);
        };

        auto *userDataDefault = new std::shared_ptr<SyncData>(syncData);
        notify_notification_add_action(n, "default", "Select and pair device", actionCb, userDataDefault, actionFree);

        g_signal_connect_data(n, "closed", G_CALLBACK(+[](NotifyNotification *, gpointer user_data) {
                                  const auto *s = static_cast<std::shared_ptr<SyncData>*>(user_data);
                                  if (!(*s)->handled.exchange(true)) {
                                  (*s)->prom.set_value(false);
                                  }
                                  }), userDataClosed, [](gpointer user_data, GClosure *)
                              {
                                  delete static_cast<std::shared_ptr<SyncData> *>(user_data);
                              }, static_cast<GConnectFlags>(0));

        bool userAccepted = false;
        GError *error = nullptr;

        if (notify_notification_show(n, &error))
        {
            std::clog << logName << "Waiting for user decision on notification for '" << config.GetDeviceName() <<
                    "'..." << std::endl;

            auto fut = syncData->prom.get_future();
            while (!Lifecycle::ShutdownHandler::ShouldShutdown())
            {
                if (fut.wait_for(std::chrono::milliseconds(200)) == std::future_status::ready)
                {
                    userAccepted = fut.get();
                    break;
                }
            }

            if (Lifecycle::ShutdownHandler::ShouldShutdown())
            {
                std::clog << logName << "Shutdown requested, closing desktop notification..." << std::endl;
                notify_notification_close(n, nullptr);
                userAccepted = false;
            }
        } else
        {
            std::cerr << logName << "Failed to show notification: " << (error ? error->message : "unknown") <<
                    std::endl;
            if (error) g_error_free(error);
        }

        g_object_unref(n);
        return userAccepted;
    }

    void NotificationManager::RequestDesktopConnectedNotification()
    {
        NotifyNotification *n = notify_notification_new("Mica: Device connected successfully!",
                                                        "The device has connected successfully.", "dialog-information");
        GError *error = nullptr;
        notify_notification_show(n, &error);
    }
}
