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
#include <sys/wait.h>
#include <arpa/inet.h>
#include <glib.h>
#include <libnotify/notify.h>
#include <poll.h>

#include "pairingmanager.hpp"
#include "../Lifecycle/shutdownhandler.hpp"
#include "../Network/Sockets/pairingsocketserver.hpp"
#include "../Network/Cryptography/ecdhkeyexchange.hpp"

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
        auto currentSecret = std::make_shared<std::vector<uint8_t>>();
        auto currentSock = std::make_shared<int>(-1);

        pairingSocketServer.Start(
            [syncPairing, currentSecret](const std::string &name, const std::string &ip, uint16_t port)
            {
                if (!syncPairing->handled.exchange(true))
                {
                    Network::NetworkConfig config(ip, port, name, std::chrono::steady_clock::now());
                    config.SetSharedSecret(*currentSecret);
                    syncPairing->prom.set_value(config);
                }
            },
            [syncPairing]()
            {
                if (!syncPairing->handled.exchange(true))
                {
                    syncPairing->prom.set_value(std::nullopt);
                }
            },
            [currentSecret, currentSock](const std::string &/*name*/, const std::string &ip, const uint16_t port) -> std::string
            {
                // Perform the TCP Handshake
                const int sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock < 0) return "";

                sockaddr_in serv_addr{};
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(port);
                if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0)
                {
                    close(sock);
                    return "";
                }

                if (connect(sock, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0)
                {
                    close(sock);
                    return "";
                }

                Network::Cryptography::EcdhKeyExchange ecdh;
                auto myPubKey = ecdh.GetPublicKey();

                std::vector<uint8_t> req(1 + myPubKey.size());
                req[0] = 0x01; // KEY_EXCHANGE_REQ
                std::ranges::copy(myPubKey, req.begin() + 1);

                if (write(sock, req.data(), req.size()) != static_cast<ssize_t>(req.size()))
                {
                    close(sock);
                    return "";
                }

                std::vector<uint8_t> resp(33);
                ssize_t bytesRead = 0;
                while (bytesRead < 33)
                {
                    const ssize_t res = read(sock, resp.data() + bytesRead, 33 - bytesRead);
                    if (res <= 0) break;
                    bytesRead += res;
                }

                if (bytesRead != 33 || resp[0] != 0x01) // KEY_EXCHANGE_RES
                {
                    close(sock);
                    return "";
                }

                const std::vector peerPubKey(resp.begin() + 1, resp.end());
                const auto secret = ecdh.ComputeSharedSecret(peerPubKey);
                if (secret.empty()) 
                {
                    close(sock);
                    return "";
                }

                *currentSecret = secret;
                *currentSock = sock; // KEEP SOCKET OPEN

                return Network::Cryptography::EcdhKeyExchange::DerivePinFromSecret(secret);
            }
        );

        GPid childPid = 0;
        GError *spawnError = nullptr;
        if (!g_spawn_async(nullptr, cArgs.data(), nullptr, G_SPAWN_DO_NOT_REAP_CHILD, nullptr, nullptr, &childPid, &spawnError))
        {
            std::cerr << logName << "Failed to spawn MicaPairing: " << (spawnError
                                                                                   ? spawnError->message
                                                                                   : "unknown") << std::endl;
            if (spawnError) g_error_free(spawnError);
            pairingSocketServer.Stop();
            return std::nullopt;
        }

        // Monitor child process exit to immediately unblock if closed or killed
        std::thread childMonitor([childPid, syncPairing]()
        {
            int status = 0;
            waitpid(childPid, &status, 0);
            g_spawn_close_pid(childPid);

            if (!syncPairing->handled.exchange(true))
            {
                syncPairing->prom.set_value(std::nullopt);
            }
        });
        childMonitor.detach();

        std::clog << logName << "Waiting for pairing decision from TUI via Unix Socket..." << std::endl;

        auto future = syncPairing->prom.get_future();
        std::optional<Network::NetworkConfig> selectedConfigOpt = std::nullopt;

        while (!Lifecycle::ShutdownHandler::ShouldShutdown())
        {
            if (future.wait_for(std::chrono::milliseconds(50)) == std::future_status::ready)
            {
                selectedConfigOpt = future.get();
                break;
            }

            int sock = *currentSock;
            if (sock != -1)
            {
                // Poll the socket for messages from Android (e.g. Reject)
                struct pollfd pfd;
                pfd.fd = sock;
                pfd.events = POLLIN;
                if (poll(&pfd, 1, 0) > 0)
                {
                    uint8_t status = 0;
                    if (read(sock, &status, 1) > 0)
                    {
                        if (status == 0xFF) // Phone rejected
                        {
                            std::clog << logName << "Phone rejected the pairing." << std::endl;
                            system("killall MicaPairing");
                            break;
                        }
                    }
                    else
                    {
                        // Socket closed unexpectedly
                        std::clog << logName << "Phone closed the connection." << std::endl;
                        system("killall MicaPairing");
                        break;
                    }
                }
            }
        }

        pairingSocketServer.Stop();

        int sock = *currentSock;
        if (sock != -1)
        {
            if (selectedConfigOpt.has_value())
            {
                // TUI Accepted. Send 0x00 to Phone
                uint8_t msg = 0x00;
                write(sock, &msg, 1);

                // Wait for Phone to reply with 0x00
                std::clog << logName << "Waiting for Phone confirmation..." << std::endl;
                struct timeval tv{};
                tv.tv_sec = 10; // Wait up to 10 seconds for user to tap on phone
                tv.tv_usec = 0;
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

                uint8_t resp = 0;
                if (read(sock, &resp, 1) > 0 && resp == 0x00)
                {
                    std::clog << logName << "Phone confirmed pairing!" << std::endl;
                }
                else
                {
                    std::clog << logName << "Phone did not confirm pairing (or rejected)." << std::endl;
                    selectedConfigOpt = std::nullopt;

                    // Let the phone know that the pairing timed out
                    msg = 0xFF;
                    write(sock, &msg, 1);
                }
            }
            else
            {
                // TUI Rejected or Cancelled. Send 0xFF to Phone
                uint8_t msg = 0xFF;
                write(sock, &msg, 1);
            }
            close(sock);
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

