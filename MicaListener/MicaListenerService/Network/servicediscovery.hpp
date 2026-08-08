/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Class for listening for DNS services using the Avahi daemon.
 */

#pragma once

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#include <string>
#include <memory>
#include <iostream>
#include <functional>
#include <utility>

#include "deviceregistry.hpp"
#include "networkconfig.hpp"
#include "../Lifecycle/shutdownhandler.hpp"

namespace MicaListener::MicaListenerService::Network
{
    using ServiceLostCallback = std::function<void(const std::string &serviceName)>;
    using ServiceResolvedCallback = std::function<void(const NetworkConfig &)>;

    class ServiceDiscovery
    {
    public:
        /// @brief Constructor for the ServiceDiscovery
        /// @param _serviceName Name of the service to look for (e.g. "_http._tcp")
        /// @param _deviceRegistry Reference to the shared device registry
        explicit ServiceDiscovery(std::string _serviceName, DeviceRegistry &_deviceRegistry);

        /// @brief Starts the discovery loop and searches for the service
        void FindService();

        /// @brief Listens for service resolution synchronously and returns the resolved NetworkConfig
        NetworkConfig ListenForService();

        /// @brief Stops the discovery loop and frees resources
        void StopService() const;

        /// @brief Sets the callback function for when a service is lost
        /// @param _callback The function to call
        void SetOnServiceLost(ServiceLostCallback _callback);

        /// @brief Sets the callback function for when a service is successfully resolved
        /// @param _callback The function to call with the resolved network config
        void SetOnServiceResolved(ServiceResolvedCallback _callback);

    private:
        /// @brief Log prefix for the Service Discovery
        static inline const std::string logName = "\033[32mDISCOVERY\033[0m\t";

        /// @brief Deleter struct for freeing the memory of the Simple Polling Loop using the C method
        struct AvahiSimplePollDeleter
        {
            void operator()(AvahiSimplePoll *_simplePoll) const;
        };

        /// @brief Deleter struct for freeing the memory of the Client using the C method
        struct AvahiClientDeleter
        {
            void operator()(AvahiClient *_client) const;
        };

        /// @brief Deleter struct for freeing the memory of the ServiceBrowser using the C method
        struct AvahiServiceBrowserDeleter
        {
            void operator()(AvahiServiceBrowser *_browser) const;
        };

        /// @brief Deleter struct for freeing the memory of the ServiceResolver using the C method
        struct AvahiServiceResolverDeleter
        {
            void operator()(AvahiServiceResolver *_resolver) const;
        };

        /// @brief Smart pointer for the Simple Poll Loop
        std::unique_ptr<AvahiSimplePoll, AvahiSimplePollDeleter> simplePoll;

        /// @brief Smart pointer for the Client
        std::unique_ptr<AvahiClient, AvahiClientDeleter> client;

        /// @brief Smart pointer for the Service Browser
        std::unique_ptr<AvahiServiceBrowser, AvahiServiceBrowserDeleter> browser;

        /// @brief Smart pointer for the Service Resolver
        std::unique_ptr<AvahiServiceResolver, AvahiServiceResolverDeleter> resolver;

        /// @brief Name of the service to search for
        std::string serviceName;

        /// @brief Device registry reference for managing active discovered devices
        DeviceRegistry &deviceRegistry;

        /// @brief Callback to main when a Service is lost
        ServiceLostCallback onServiceLost;

        /// @brief Callback to main when the Service is resolved
        ServiceResolvedCallback onServiceResolved;

        /// @brief Internal helper to create the Avahi Simple Poll Loop
        void CreateSimplePollLoop();

        /// @brief Internal helper to create the Avahi Client
        void CreateClient();

        /// @brief Internal helper to create the Service Browser
        /// @param _client The active Avahi client used to create the browser
        void CreateServiceBrowser(AvahiClient *_client);

        /// @brief Internal helper to create a Service Resolver for a found service
        /// @param _client The active Avahi client
        /// @param _interface The interface index where the service was found
        /// @param _protocol The protocol (IPv4/IPv6) of the service
        /// @param _name The name of the service
        /// @param _type The type of the service (e.g. _http._tcp)
        /// @param _domain The domain of the service (e.g. local)
        void CreateServiceResolver(AvahiClient *_client, const AvahiIfIndex _interface, const AvahiProtocol _protocol,
                                   const char *_name, const char *_type, const char *_domain);

        /// @brief Static C-style callback wrapper for client state changes
        /// @param _client The Avahi client instance
        /// @param _state The new state of the client
        /// @param _userData Pointer to the ServiceDiscovery instance (this)
        static void ClientCallback(AvahiClient *_client, AvahiClientState _state, void *_userData);

        /// @brief Member function to handle client state changes
        /// @param _client The Avahi client instance
        /// @param _state The new state of the client
        void HandleClientState(AvahiClient *_client, AvahiClientState _state);

        /// @brief Static C-style callback wrapper for browser events
        /// @param _browser The service browser instance
        /// @param _interface The interface index where the event occurred
        /// @param _protocol The protocol of the service
        /// @param _event The browser event (e.g. ADDED, REMOVED)
        /// @param _name The name of the service
        /// @param _type The type of the service
        /// @param _domain The domain of the service
        /// @param _flags Lookup flags
        /// @param _userData Pointer to the ServiceDiscovery instance (this)
        static void BrowseCallback(AvahiServiceBrowser *_browser, AvahiIfIndex _interface, AvahiProtocol _protocol,
                                   const AvahiBrowserEvent _event, const char *_name, const char *_type,
                                   const char *_domain,
                                   AvahiLookupResultFlags _flags, void *_userData);

        /// @brief Member function to handle browser events (Service found/removed)
        /// @param _event The browser event (e.g. ADDED, REMOVED)
        /// @param _name The name of the service
        /// @param _type The type of the service
        /// @param _domain The domain of the service
        void HandleBrowserState(const AvahiBrowserEvent _event, const char *_name, const char *_type,
                                const char *_domain);

        /// @brief Static C-style callback wrapper for resolver events
        /// @param _resolver The service resolver instance
        /// @param _interface The interface index
        /// @param _protocol The protocol
        /// @param _event The resolver event (e.g. FOUND, FAILURE)
        /// @param _name The name of the service
        /// @param _type The type of the service
        /// @param _domain The domain of the service
        /// @param _hostName The hostname of the service
        /// @param _address The address of the service
        /// @param _port The port of the service
        /// @param _text The TXT record data
        /// @param _flags Lookup flags
        /// @param _userData Pointer to the ServiceDiscovery instance (this)
        static void ResolveCallback(AvahiServiceResolver *_resolver, AvahiIfIndex _interface, AvahiProtocol _protocol,
                                    AvahiResolverEvent _event, const char *_name, const char *_type,
                                    const char *_domain, const char *_hostName, const AvahiAddress *_address,
                                    uint16_t _port, AvahiStringList *_text, AvahiLookupResultFlags _flags,
                                    void *_userData);

        /// @brief Member function to handle resolver events (Service resolved/failure)
        /// @param _event The resolver event (e.g. FOUND, FAILURE)
        /// @param _name The name of the service
        /// @param _address The resolved address of the service
        /// @param _port The resolved port of the service
        void HandleResolverState(AvahiResolverEvent _event, const char *_name, const AvahiAddress *_address,
                                 uint16_t _port) const;
    };
}
