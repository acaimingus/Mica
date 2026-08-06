/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Implementation of NotificationManager using libnotify and GLib.
 */

#include <atomic>
#include <chrono>
#include <climits>
#include <filesystem>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <glib.h>
#include <libnotify/notify.h>

#include "pairingmanager.hpp"
#include "../Network/Sockets/pairingsocketserver.hpp"

namespace MicaListener::MicaListenerService::Pairing
{
    void PairingManager::Initialize()
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

    bool PairingManager::RequestDesktopNotification(const Network::NetworkConfig &config)
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

        auto actionFree = [](gpointer user_data)
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
            userAccepted = syncData->prom.get_future().get();
        } else
        {
            std::cerr << logName << "Failed to show notification: " << (error ? error->message : "unknown") <<
                    std::endl;
            if (error) g_error_free(error);
        }

        g_object_unref(n);
        return userAccepted;
    }

    std::string PairingManager::GetExecutableDir()
    {
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        if (count != -1)
        {
            std::filesystem::path p(std::string(result, count));
            return p.parent_path().string();
        }
        return ".";
    }

    /// @brief Handles desktop notification and IPC pairing confirmation for a newly discovered device
    /// @param newDeviceConfig The network configuration of the newly discovered service
    /// @return std::optional<Network::NetworkConfig> containing the user-selected device config, or std::nullopt if ignored/cancelled
    std::optional<Network::NetworkConfig> PairingManager::HandleDevicePairing(
        const Network::NetworkConfig &newDeviceConfig, const Network::DeviceRegistry &deviceRegistry)
    {
        // Socket server for handling pairing messages from the pairing service (accept/deny)
        Network::Sockets::PairingSocketServer pairingSocketServer;

        const bool doPair = Pairing::PairingManager::RequestDesktopNotification(newDeviceConfig);

        if (!doPair)
        {
            std::clog << logName << "User ignored notification. Skipping connection." << std::endl;
            return std::nullopt;
        }

        std::clog << logName << "User selected Pair. Executing MicaPairing..." << std::endl;
        const std::string exePath = GetExecutableDir() + "/MicaPairing";

        std::vector<std::string> args = {exePath};
        for (const auto activeDevices = deviceRegistry.GetActiveDevices(); const auto &dev: activeDevices)
        {
            args.push_back(dev.GetDeviceName());
            args.push_back(dev.GetIp());
            args.push_back(std::to_string(dev.GetPort()));
        }

        std::vector<char *> cArgs;
        for (auto &a: args) cArgs.push_back(a.data());
        cArgs.push_back(nullptr);

        struct SyncPairing
        {
            std::promise<std::optional<Network::NetworkConfig> > prom;
            std::atomic<bool> handled{false};
        };
        auto syncPairing = std::make_shared<SyncPairing>();

        pairingSocketServer.Start(
            [syncPairing](const std::string &name, const std::string &ip, uint16_t port)
            {
                if (!syncPairing->handled.exchange(true))
                {
                    syncPairing->prom.set_value(
                        Network::NetworkConfig(ip, port, name, std::chrono::steady_clock::now()));
                }
            },
            [syncPairing]()
            {
                if (!syncPairing->handled.exchange(true))
                {
                    syncPairing->prom.set_value(std::nullopt);
                }
            }
        );

        GError *spawnError = nullptr;
        if (!g_spawn_async(nullptr, cArgs.data(), nullptr, G_SPAWN_DEFAULT, nullptr, nullptr, nullptr, &spawnError))
        {
            std::cerr << logName << "Failed to spawn MicaPairing: " << (spawnError
                                                                                   ? spawnError->message
                                                                                   : "unknown") << std::endl;
            if (spawnError) g_error_free(spawnError);
            pairingSocketServer.Stop();
            return std::nullopt;
        }

        std::clog << logName << "Waiting for pairing decision from TUI via Unix Socket..." << std::endl;

        auto future = syncPairing->prom.get_future();
        std::optional<Network::NetworkConfig> selectedConfigOpt = std::nullopt;

        if (future.wait_for(std::chrono::minutes(1)) == std::future_status::ready)
        {
            selectedConfigOpt = future.get();
        } else
        {
            std::cerr << logName << "Timed out waiting for pairing decision." << std::endl;
        }

        pairingSocketServer.Stop();

        if (selectedConfigOpt.has_value())
        {
            std::clog << logName << "Device selected via Unix Socket: " << selectedConfigOpt->GetDeviceName() <<
                    std::endl;
        } else
        {
            std::clog << logName << "Pairing cancelled or rejected." << std::endl;
        }

        return selectedConfigOpt;
    }
}
