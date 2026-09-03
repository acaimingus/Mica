/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Implementation of NotificationManager using libnotify and GLib.
 */

#include "pairingmanager.hpp"

namespace MicaListener::MicaListenerService::Pairing
{
    std::string PairingManager::GetExecutableDir()
    {
        char result[PATH_MAX];
        const ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        if (count != -1)
        {
            const std::filesystem::path p(std::string(result, count));
            return p.parent_path().string();
        }
        return ".";
    }

    std::optional<Network::NetworkConfig> PairingManager::HandleDevicePairing(
        const Network::NetworkConfig &newDeviceConfig, const Network::DeviceRegistry &deviceRegistry)
    {
        // Socket server for handling pairing messages from the pairing service (accept/deny)
        Network::Sockets::PairingSocketServer pairingSocketServer;

        const bool doPair = Notification::NotificationManager::RequestDesktopPairingNotification(newDeviceConfig);

        if (!doPair)
        {
            std::clog << logName << "User ignored notification. Skipping connection." << std::endl;
            return std::nullopt;
        }

        std::clog << logName << "User selected Pair. Executing mica-pairing..." << std::endl;
        const std::string exePath = GetExecutableDir() + "/mica-pairing";

        std::vector args = {exePath};
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
            std::cerr << logName << "Failed to spawn mica-pairing: " << (spawnError
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

            if (WIFEXITED(status) && WEXITSTATUS(status) == 10)
            {
                std::cerr << logName << "Failed to show pairing UI: No terminal emulator found on the system." << std::endl;
            }

            if (!syncPairing->handled.exchange(true))
            {
                syncPairing->prom.set_value(std::nullopt);
            }
        });
        childMonitor.detach();

        std::clog << logName << "Waiting for pairing decision from TUI via Unix Socket..." << std::endl;

        auto future = syncPairing->prom.get_future();
        std::optional<Network::NetworkConfig> selectedConfigOpt = std::nullopt;
        std::optional<uint8_t> phoneResponse = std::nullopt;

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
                            system("killall mica-pairing");
                            break;
                        }
                        if (status == 0x00) // Phone accepted early
                        {
                            phoneResponse = status;
                        }
                    }
                    else
                    {
                        // Socket closed unexpectedly
                        std::clog << logName << "Phone closed the connection." << std::endl;
                        system("killall mica-pairing");
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
                bool gotResp = false;
                if (phoneResponse.has_value())
                {
                    resp = phoneResponse.value();
                    gotResp = true;
                }
                else
                {
                    gotResp = (read(sock, &resp, 1) > 0);
                }

                if (gotResp && resp == 0x00)
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

