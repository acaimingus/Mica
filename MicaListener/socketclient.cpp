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
        SocketClient(const NetworkConfig &config)
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
            server.sin_port = htons(config.GetPort());

            // Change IP address from string to binary form
            if (inet_pton(AF_INET, config.GetIp().c_str(), &server.sin_addr) <= 0)
            {
                close(sock);
                throw std::runtime_error("Invalid address / Address not supported: " + config.GetIp());
            }

            std::cout << "Connecting to " << config.GetIp() << ":" << config.GetPort() << "..." << std::endl;

            // Connect
            if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
            {
                std::string errorMsg = strerror(errno);
                // Clean up before throwing exception!
                close(sock); 
                throw std::runtime_error("Connection failed: " + errorMsg);
            }

            std::cout << "Connected!" << std::endl;
        }

        ~SocketClient()
        {
            if (sock != -1)
            {
                std::cout << "Closing socket..." << std::endl;
                close(sock);
            }
        }

        ssize_t Read(std::vector<uint8_t>& buffer)
        {
            ssize_t bytesRead = recv(sock, buffer.data(), buffer.size(), 0);
            
            if (bytesRead == 0) {
                std::cout << "Server closed connection." << std::endl;
            } else if (bytesRead < 0) {
                std::cerr << "Read error: " << strerror(errno) << std::endl;
            }
            
            return bytesRead;
        }
    private:
        std::string ip;
        int port;
        int sock;
    };
}
