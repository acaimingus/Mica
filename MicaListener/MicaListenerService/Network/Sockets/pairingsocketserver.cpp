/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Thread-safe Unix Domain Socket server implementation for receiving pairing selection messages from
 * MicaPairingService.
 */

#include "pairingsocketserver.hpp"

namespace MicaListener::MicaListenerService::Network::Sockets
{
PairingSocketServer::~PairingSocketServer()
{
    Stop();
}

bool PairingSocketServer::Start(ConfirmCallback onConfirm, CancelCallback onCancel, ExchangeCodeCallback onExchangeCode)
{
    confirmCb = std::move(onConfirm);
    cancelCb = std::move(onCancel);
    exchangeCodeCb = std::move(onExchangeCode);
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

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
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

void PairingSocketServer::Stop()
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
        connect(dummyFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
        close(dummyFd);
    }

    if (serverThread.joinable())
    {
        serverThread.join();
    }
    unlink(socketPath);
}

void PairingSocketServer::TrimWhitespace(std::string &s)
{
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' '))
    {
        s.pop_back();
    }
}

void PairingSocketServer::ListenLoop() const
{
    while (isRunning)
    {
        int clientFd = accept(serverFd, nullptr, nullptr);
        if (clientFd < 0)
        {
            if (!isRunning)
                break;
            continue;
        }

        char buffer[512];
        const ssize_t bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);

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

                        std::clog << logName << "Received PAIR command: " << devName << " (" << ip << ":" << port << ")"
                                  << std::endl;
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
                else if (command == "EXCHANGE_CODE")
                {
                    std::string devName, ip, portStr;
                    if (std::getline(ss, devName) && std::getline(ss, ip) && std::getline(ss, portStr))
                    {
                        TrimWhitespace(devName);
                        TrimWhitespace(ip);
                        TrimWhitespace(portStr);
                        const auto port = static_cast<uint16_t>(std::stoi(portStr));

                        std::clog << logName << "Received EXCHANGE_CODE command: " << devName << " (" << ip << ":"
                                  << port << ")" << std::endl;
                        if (exchangeCodeCb)
                        {
                            std::string pin = exchangeCodeCb(devName, ip, port);
                            write(clientFd, pin.c_str(), pin.length());
                        }
                    }
                }
            }
        }
        close(clientFd);
    }
}
} // namespace MicaListener::MicaListenerService::Network::Sockets