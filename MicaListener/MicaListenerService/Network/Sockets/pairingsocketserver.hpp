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
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

namespace MicaListener::MicaListenerService::Network::Sockets
{
/// @brief Thread-safe Unix Domain Socket server for receiving IPC pairing commands from the pairing TUI/service
class PairingSocketServer
{
  public:
    /// @brief Callback invoked when a PAIR command is confirmed
    using ConfirmCallback = std::function<void(const std::string &name, const std::string &ip, uint16_t port)>;

    /// @brief Callback invoked when a CANCEL command is received
    using CancelCallback = std::function<void()>;

    /// @brief Callback invoked when an EXCHANGE_CODE command is received, returning the verification PIN
    using ExchangeCodeCallback =
        std::function<std::string(const std::string &name, const std::string &ip, uint16_t port)>;

    /// @brief Filesystem path for the Unix Domain Socket
    static constexpr auto socketPath = "/tmp/mica_pairing.sock";

    /// @brief Constructor
    PairingSocketServer() = default;

    /// @brief Destructor, stops the server and cleans up resources
    ~PairingSocketServer();

    /// @brief Starts the Unix Domain Socket server and listens for incoming pairing messages
    /// @param onConfirm Callback when pairing is confirmed
    /// @param onCancel Callback when pairing is cancelled
    /// @param onExchangeCode Optional callback for PIN/code exchange
    /// @return True if the server started successfully, false otherwise
    bool Start(ConfirmCallback onConfirm, CancelCallback onCancel, ExchangeCodeCallback onExchangeCode = nullptr);

    /// @brief Stops the server thread and cleans up the socket
    void Stop();

  private:
    /// @brief Log prefix for the IPC server
    static constexpr std::string_view logName = "\033[36mIPC-SERVER\033[0m\t";

    /// @brief File descriptor for the listening server socket
    int serverFd{-1};

    /// @brief Atomic flag indicating whether the server loop is running
    std::atomic<bool> isRunning{false};

    /// @brief Thread running the accept/listen loop
    std::thread serverThread;

    /// @brief Registered callback for PAIR command
    ConfirmCallback confirmCb;

    /// @brief Registered callback for CANCEL command
    CancelCallback cancelCb;

    /// @brief Registered callback for EXCHANGE_CODE command
    ExchangeCodeCallback exchangeCodeCb;

    /// @brief Helper method to strip trailing whitespace and newlines from strings
    /// @param s String to be trimmed in-place
    static void TrimWhitespace(std::string &s);

    /// @brief Worker loop accepting client connections and processing commands
    void ListenLoop() const;
};
} // namespace MicaListener::MicaListenerService::Network::Sockets
