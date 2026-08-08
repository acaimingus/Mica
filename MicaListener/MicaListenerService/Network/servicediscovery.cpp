/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Class for listening for DNS services using the Avahi daemon.
 */

#include "servicediscovery.hpp"

namespace MicaListener::MicaListenerService::Network
{
    using ServiceLostCallback = std::function<void(const std::string &serviceName)>;
    using ServiceResolvedCallback = std::function<void(const NetworkConfig &)>;

    ServiceDiscovery::ServiceDiscovery(std::string _serviceName, DeviceRegistry &_deviceRegistry) : serviceName(std::move(_serviceName)), deviceRegistry(_deviceRegistry)
    {
        CreateSimplePollLoop();
    }

    void ServiceDiscovery::FindService()
    {
        CreateClient();

        // Instead of using the normal poll loop manually loop while checking if the application should shut down
        while (!Lifecycle::ShutdownHandler::ShouldShutdown())
        {
            if (avahi_simple_poll_iterate(simplePoll.get(), 500) != 0)
            {
                break;
            }
        }
    }

    NetworkConfig ServiceDiscovery::ListenForService()
    {
        std::string foundIp;
        int foundPort = 0;
        std::string foundName;

        SetOnServiceResolved(
            [&](const NetworkConfig &_config)
            {
                foundIp = _config.GetIp();
                foundPort = _config.GetPort();
                foundName = _config.GetDeviceName();
                std::clog << logName << "Received Service info: " << foundIp << " / " << foundPort << std::endl;
                StopService();
            });

        std::clog << logName << "Looking for services..." << std::endl;
        FindService();

        return NetworkConfig(foundIp, foundPort, foundName, std::chrono::steady_clock::now());
    }

    void ServiceDiscovery::StopService() const
    {
        if (simplePoll)
        {
            avahi_simple_poll_quit(simplePoll.get());
        }
    }

    void ServiceDiscovery::SetOnServiceLost(ServiceLostCallback _callback)
    {
        onServiceLost = std::move(_callback);
    }

    void ServiceDiscovery::SetOnServiceResolved(ServiceResolvedCallback _callback)
    {
        onServiceResolved = std::move(_callback);
    }

    void ServiceDiscovery::AvahiSimplePollDeleter::operator()(AvahiSimplePoll *_simplePoll) const
    {
        avahi_simple_poll_free(_simplePoll);
    }

    void ServiceDiscovery::AvahiClientDeleter::operator()(AvahiClient *_client) const
    {
        avahi_client_free(_client);
    }

    void ServiceDiscovery::AvahiServiceBrowserDeleter::operator()(AvahiServiceBrowser *_browser) const
    {
        avahi_service_browser_free(_browser);
    }

    void ServiceDiscovery::AvahiServiceResolverDeleter::operator()(AvahiServiceResolver *_resolver) const
    {
        avahi_service_resolver_free(_resolver);
    }

    void ServiceDiscovery::CreateSimplePollLoop()
    {
        std::clog << logName << "Creating the Simple Poll Loop..." << std::endl;
        // Create a simple pool loop
        simplePoll.reset(avahi_simple_poll_new());
        if (!simplePoll)
        {
            std::cerr << logName << "Failed to create the Avahi Simple Poll loop!" << std::endl;
            return;
        }
    }

    void ServiceDiscovery::CreateClient()
    {
        std::clog << logName << "Creating the Client..." << std::endl;
        int error = 0;
        client.reset(avahi_client_new(avahi_simple_poll_get(simplePoll.get()), static_cast<AvahiClientFlags>(0),
                                      ClientCallback, this, &error));
        std::clog << logName << "Asserting that the Client exists..." << std::endl;
        if (error != 0)
        {
            std::cerr << logName << "Failed to create the Avahi Client!" << avahi_strerror(error) << std::endl;
            avahi_simple_poll_quit(simplePoll.get());
        }
    }

    void ServiceDiscovery::CreateServiceBrowser(AvahiClient *_client)
    {
        std::clog << logName << "Creating the Browser..." << std::endl;
        // Create the Service Browser
        // Only browse for IPv4 services for simplicity
        browser.reset(avahi_service_browser_new(_client, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET,
                                                serviceName.c_str(), nullptr, static_cast<AvahiLookupFlags>(0),
                                                BrowseCallback, this));
        std::clog << logName << "Asserting that the Browser exists..." << std::endl;
        if (!browser)
        {
            std::cerr << logName << "Failed to create the Avahi Service Browser!" << std::endl;
            avahi_simple_poll_quit(simplePoll.get());
        }
    }

    void ServiceDiscovery::CreateServiceResolver(AvahiClient *_client, const AvahiIfIndex _interface,
                                                 const AvahiProtocol _protocol,
                                                 const char *_name, const char *_type, const char *_domain)
    {
        std::clog << logName << "Creating the Resolver..." << std::endl;
        // Create the Service Resolver
        // Only resolve for IPv4 services for simplicity
        resolver.reset(avahi_service_resolver_new(_client, _interface, _protocol, _name, _type, _domain,
                                                  AVAHI_PROTO_INET, static_cast<AvahiLookupFlags>(0),
                                                  ResolveCallback, this));
        if (!resolver)
        {
            std::cerr << logName << "Failed to creat the Avahi Service Resolver" << std::endl;
            avahi_simple_poll_quit(simplePoll.get());
        }
    }

    void ServiceDiscovery::ClientCallback(AvahiClient *_client, AvahiClientState _state, void *_userData)
    {
        auto *self = static_cast<ServiceDiscovery *>(_userData);
        std::clog << logName << "Client Callback: ";
        self->HandleClientState(_client, _state);
    }

    void ServiceDiscovery::HandleClientState(AvahiClient *_client, AvahiClientState _state)
    {
        switch (_state)
        {
            case AVAHI_CLIENT_FAILURE:
                avahi_simple_poll_quit(simplePoll.get());
                std::cerr << "Failed to connect to the Avahi Client!" << std::endl;
                break;
            case AVAHI_CLIENT_S_RUNNING:
                std::clog << "Client is running!" << std::endl;
                CreateServiceBrowser(_client);
                break;
            default:
                break;
        }
    }

    void ServiceDiscovery::BrowseCallback([[maybe_unused]] AvahiServiceBrowser *_browser, [[maybe_unused]] AvahiIfIndex _interface,
                                          [[maybe_unused]] AvahiProtocol _protocol,
                                          const AvahiBrowserEvent _event, const char *_name, const char *_type,
                                          const char *_domain,
                                          [[maybe_unused]] AvahiLookupResultFlags _flags, void *_userData)
    {
        auto *self = static_cast<ServiceDiscovery *>(_userData);
        std::clog << logName << "Browser Callback: ";
        self->HandleBrowserState(_event, _name, _type, _domain);
    }

    void ServiceDiscovery::HandleBrowserState(const AvahiBrowserEvent _event, const char *_name, const char *_type,
                                              const char *_domain)
    {
        switch (_event)
        {
            case AVAHI_BROWSER_FAILURE:
                avahi_simple_poll_quit(simplePoll.get());
                std::cerr << "The Avahi Browser failed!" << std::endl;
                break;
            case AVAHI_BROWSER_NEW:
                // Log the found service
                std::clog << "Found service: " << (_name ? _name : "(null)")
                        << " type: " << (_type ? _type : "(null)")
                        << " domain: " << (_domain ? _domain : "(null)") << std::endl;

                // Resolve the found service
                CreateServiceResolver(client.get(), AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
                                      _name, _type, _domain);
                break;
            case AVAHI_BROWSER_REMOVE:
                std::clog << "Removed service: " << (_name ? _name : "(null)")
                        << " type: " << (_type ? _type : "(null)")
                        << " domain: " << (_domain ? _domain : "(null)") << std::endl;
                if (_name != nullptr)
                {
                    deviceRegistry.RemoveDevice(_name);
                }
                if (onServiceLost)
                {
                    onServiceLost(_name);
                }
                break;
            case AVAHI_BROWSER_ALL_FOR_NOW:
                std::clog << "Avahi Browser has found every entry for now." << std::endl;
                break;
            case AVAHI_BROWSER_CACHE_EXHAUSTED:
                std::clog << "Avahi Cache was exhausted." << std::endl;
                break;
        }
    }

    void ServiceDiscovery::ResolveCallback([[maybe_unused]] AvahiServiceResolver *_resolver, [[maybe_unused]] AvahiIfIndex _interface,
                                           [[maybe_unused]] AvahiProtocol _protocol,
                                           AvahiResolverEvent _event, const char *_name, [[maybe_unused]] const char *_type,
                                           [[maybe_unused]] const char *_domain, [[maybe_unused]] const char *_hostName, const AvahiAddress *_address,
                                           uint16_t _port, [[maybe_unused]] AvahiStringList *_text, [[maybe_unused]] AvahiLookupResultFlags _flags,
                                           void *_userData)
    {
        const auto *self = static_cast<ServiceDiscovery *>(_userData);
        std::clog << logName << "Resolve Callback: ";
        self->HandleResolverState(_event, _name, _address, _port);
    }

    void ServiceDiscovery::HandleResolverState(AvahiResolverEvent _event, const char *_name,
                                               const AvahiAddress *_address,
                                               uint16_t _port) const
    {
        switch (_event)
        {
            case AVAHI_RESOLVER_FAILURE:
                std::cerr << "Avahi Resolver failed!" << std::endl;
                break;
            case AVAHI_RESOLVER_FOUND:
                char address[AVAHI_ADDRESS_STR_MAX];
                avahi_address_snprint(address, sizeof(address), _address);
                std::clog << "Avahi Service Resolver found the Service: " << _name << " / " << address << " / " <<
                        _port << std::endl;
                const NetworkConfig networkConfig(address, _port, _name ? _name : "", std::chrono::steady_clock::now());
                onServiceResolved(networkConfig);
                break;
        }
    }
}
