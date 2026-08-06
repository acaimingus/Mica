/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Thread-safe Unix Domain Socket server for receiving pairing selection messages from MicaPairingService.
 */

#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace MicaListener::MicaListenerService::Network::Sockets
{
    class PairingSocketServer
    {
    public:
        using ConfirmCallback = std::function<void(const std::string &name, const std::string &ip, uint16_t port)>;
        using CancelCallback = std::function<void()>;

        static constexpr const char* socketPath = "/tmp/mica_pairing.sock";

        PairingSocketServer() = default;
        ~PairingSocketServer() { Stop(); }

        bool Start(ConfirmCallback onConfirm, CancelCallback onCancel)
        {
            confirmCb = std::move(onConfirm);
            cancelCb = std::move(onCancel);
            isRunning = true;

            unlink(socketPath);

            serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (serverFd < 0)
            {
                std::cerr << logName << "Failed to create Unix domain socket." << std::endl;
                return false;
            }

            sockaddr_un addr{};
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);

            if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
            {
                std::cerr << logName << "Failed to bind Unix domain socket to " << socketPath << std::endl;
                close(serverFd);
                serverFd = -1;
                return false;
            }

            if (listen(serverFd, 5) < 0)
            {
                std::cerr << logName << "Failed to listen on Unix domain socket." << std::endl;
                close(serverFd);
                serverFd = -1;
                return false;
            }

            std::clog << logName << "Unix socket server listening on " << socketPath << std::endl;

            serverThread = std::thread([this]() { ListenLoop(); });
            return true;
        }

        void Stop()
        {
            if (!isRunning.exchange(false))
            {
                return;
            }

            if (serverFd >= 0)
            {
                shutdown(serverFd, SHUT_RDWR);
                close(serverFd);
                serverFd = -1;
            }

            // Connect a dummy client to unblock any pending accept() in serverThread
            int dummyFd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (dummyFd >= 0)
            {
                sockaddr_un addr{};
                addr.sun_family = AF_UNIX;
                strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);
                connect(dummyFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
                close(dummyFd);
            }

            if (serverThread.joinable())
            {
                serverThread.join();
            }
            unlink(socketPath);
        }

    private:
        static constexpr std::string_view logName = "\033[36mIPC-SERVER\033[0m\t";

        int serverFd{-1};
        std::atomic<bool> isRunning{false};
        std::thread serverThread;
        ConfirmCallback confirmCb;
        CancelCallback cancelCb;

        static void TrimWhitespace(std::string &s)
        {
            while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' '))
            {
                s.pop_back();
            }
        }

        void ListenLoop()
        {
            while (isRunning)
            {
                int clientFd = accept(serverFd, nullptr, nullptr);
                if (clientFd < 0)
                {
                    if (!isRunning) break;
                    continue;
                }

                char buffer[512];
                const ssize_t bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);
                close(clientFd);

                if (bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    std::stringstream ss(buffer);
                    if (std::string command; std::getline(ss, command))
                    {
                        TrimWhitespace(command);

                        if (command == "PAIR")
                        {
                            std::string devName, ip, portStr;
                            if (std::getline(ss, devName) && std::getline(ss, ip) && std::getline(ss, portStr))
                            {
                                TrimWhitespace(devName);
                                TrimWhitespace(ip);
                                TrimWhitespace(portStr);
                                const auto port = static_cast<uint16_t>(std::stoi(portStr));

                                std::clog << logName << "Received PAIR command: " << devName << " (" << ip << ":" << port << ")" << std::endl;
                                if (confirmCb)
                                {
                                    confirmCb(devName, ip, port);
                                }
                            }
                        }
                        else if (command == "CANCEL")
                        {
                            std::clog << logName << "Received CANCEL command." << std::endl;
                            if (cancelCb)
                            {
                                cancelCb();
                            }
                        }
                    }
                }
            }
        }
    };
}
