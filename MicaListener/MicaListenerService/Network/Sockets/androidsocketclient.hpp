/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Class for managing socket connections.
 */

#pragma once

#include <arpa/inet.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <cstring>
#include <vector>

#include "../../Audio/sinkmanager.hpp"
#include "../../Audio/audioplayer.hpp"
#include "../../Lifecycle/shutdownhandler.hpp"
#include "../networkconfig.hpp"

namespace MicaListener::MicaListenerService::Network::Sockets
{
    class AndroidSocketClient
    {
    public:
        /// @brief Constructor of the SocketClient class, takes a NetworkConfig struct and creates a socket based on it
        explicit AndroidSocketClient(const NetworkConfig &_config);

        /// @brief Destructor for the SocketClient class
        ~AndroidSocketClient();

        /// @brief Method for reading data from the socket
        ssize_t Read(std::vector<uint8_t> &_buffer) const;

        /// @brief Creates a virtual PulseAudio sink, connects to the given service and
        ///        streams received audio data until the connection is lost or shutdown is requested
        /// @param _config The network address and port of the discovered Mica service
        static void ConnectToService(const Network::NetworkConfig &_config);
    private:
        /// @brief Log prefix for the Socket Client
        static inline const std::string logName = "\033[34mSOCKET\033[0m\t\t";

        /// @brief Socket for the connection with the app
        int sock;
    };
}
