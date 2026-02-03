#pragma once

#include <arpa/inet.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <cstring>
#include <vector>

#include "networkconfig.hpp"

namespace MicaListener
{
    class SocketClient
    {
    public:
        /// @brief Constructor of the SocketClient class, takes a NetworkConfig struct and creates a socket based on it
        explicit SocketClient(const NetworkConfig &_config);

        /// @brief Destructor for the SocketClient class
        ~SocketClient();

        /// @brief Method for reading data from the socket
        ssize_t Read(std::vector<uint8_t>& _buffer) const;

    private:
        /// @brief Log prefix for the Socket Client
        static inline const std::string logName = "\033[34mSOCKET\033[0m\t\t";

        /// @brief Socket for the connection with the app
        int sock;
    };
}
