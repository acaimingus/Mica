#include <sys/socket.h>
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
        SocketClient(const NetworkConfig &_config)
        {
            // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = IP
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == -1)
            {
                throw std::runtime_error("Could not create socket: " + std::string(strerror(errno)));
            }

            // Prepare the IP address
            sockaddr_in server;
            server.sin_family = AF_INET;
            // Change port to network byte order (Endianness)
            server.sin_port = htons(_config.GetPort());

            // Change IP address from string to binary form
            if (inet_pton(AF_INET, _config.GetIp().c_str(), &server.sin_addr) <= 0)
            {
                close(sock);
                throw std::runtime_error("Invalid address / Address not supported: " + _config.GetIp());
            }

            std::clog << logName << "Connecting to " << _config.GetIp() << ":" << _config.GetPort() << "..." << std::endl;

            // Connect
            if (connect(sock, reinterpret_cast<struct sockaddr *>(&server), sizeof(server)) < 0)
            {
                std::string errorMsg = strerror(errno);
                // Clean up before throwing exception!
                close(sock); 
                throw std::runtime_error("Connection failed: " + errorMsg);
            }

            std::clog << logName<< "Connected!" << std::endl;
        }

        ~SocketClient()
        {
            if (sock != -1)
            {
                std::clog << logName << "Closing socket..." << std::endl;
                close(sock);
            }
        }

        ssize_t Read(std::vector<uint8_t>& _buffer) const {
            ssize_t bytesRead = recv(sock, _buffer.data(), _buffer.size(), 0);
            
            if (bytesRead == 0) {
                std::clog << logName << "Server closed connection." << std::endl;
            } else if (bytesRead < 0) {
                std::cerr << logName << "Read error: " << strerror(errno) << std::endl;
            }
            
            return bytesRead;
        }
    private:
        /// @brief Log prefix for the Socket Client
        static inline const std::string logName = "\033[34mSOCKET\033[0m\t\t";

        std::string ip;
        int port;
        int sock;
    };
}
