#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>

#include <string>
#include <memory>
#include <iostream>
#include <functional>
#include <utility>

#include "networkconfig.hpp"
#include "shutdownhandler.cpp"

namespace MicaListener
{
    using ServiceLostCallback = std::function<void(const std::string &serviceName)>;
    using ServiceResolvedCallback = std::function<void(const MicaListener::NetworkConfig &)>;

    class ServiceDiscovery
    {
    public:
        explicit ServiceDiscovery(std::string _serviceName) : serviceName(std::move(_serviceName))
        {
            CreateSimplePollLoop();
        }

        void FindService()
        {
            CreateClient();

            // Instead of using the normal poll loop manually loop while checking if the application should shut down
            while (!ShutdownHandler::ShouldShutdown())
            {
                if (avahi_simple_poll_iterate(simplePoll.get(), 500) != 0)
                {
                    break;
                }
            }
        }

        void StopService() const
        {
            if (simplePoll)
            {
                avahi_simple_poll_quit(simplePoll.get());
            }
        }

        void SetOnServiceLost(ServiceLostCallback _callback)
        {
            onServiceLost = std::move(_callback);
        }

        void SetOnServiceResolved(ServiceResolvedCallback _callback)
        {
            onServiceResolved = std::move(_callback);
        }

    private:
        /// @brief Log prefix for the Service Discovery
        static inline const std::string logName = "\033[32mDISCOVERY\033[0m\t";

        /// @brief Deleter struct for freeing the memory of the Simple Polling Loop using the C method
        struct AvahiSimplePollDeleter
        {
            void operator()(AvahiSimplePoll *_simplePoll) const
            {
                avahi_simple_poll_free(_simplePoll);
            }
        };

        /// @brief Deleter struct for freeing the memory of the Client using the C method
        struct AvahiClientDeleter
        {
            void operator()(AvahiClient *_client) const
            {
                avahi_client_free(_client);
            }
        };

        /// @brief Deleter struct for freeing the memory of the ServiceBrowser using the C method
        struct AvahiServiceBrowserDeleter
        {
            void operator()(AvahiServiceBrowser *_browser) const
            {
                avahi_service_browser_free(_browser);
            }
        };

        /// @brief Deleter struct for freeing the memory of the ServiceResolver using the C method
        struct AvahiServiceResolverDeleter
        {
            void operator()(AvahiServiceResolver *_resolver) const
            {
                avahi_service_resolver_free(_resolver);
            }
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

        /// @brief Callback to main when a Service is lost
        ServiceLostCallback onServiceLost;

        /// @brief Callback to main when the Service is resolved
        ServiceResolvedCallback onServiceResolved;

        void CreateSimplePollLoop()
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

        void CreateClient()
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
                return;
            }
        }

        void CreateServiceBrowser(AvahiClient *_client)
        {
            std::clog << logName << "Creating the Browser..." << std::endl;
            // Create the Service Browser
            // Only browse for IPv4 services for simplicity
            browser.reset(avahi_service_browser_new(_client, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, serviceName.c_str(),
                                                    nullptr, static_cast<AvahiLookupFlags>(0), BrowseCallback, this));
            std::clog << logName << "Asserting that the Browser exists..." << std::endl;
            if (!browser)
            {
                std::cerr << logName << "Failed to create the Avahi Service Browser!" << std::endl;
                avahi_simple_poll_quit(simplePoll.get());
            }
        }

        void CreateServiceResolver(AvahiClient *_client, AvahiIfIndex _interface, const AvahiProtocol _protocol,
                                   const char *_name, const char *_type, const char *_domain)
        {
            // There is already a resolver
            if (resolver)
            {
            }

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

        static void ClientCallback(AvahiClient *_client, AvahiClientState _state, void *_userData)
        {
            auto *self = static_cast<ServiceDiscovery *>(_userData);
            std::clog << MicaListener::ServiceDiscovery::logName << "Client Callback: ";
            self->HandleClientState(_client, _state);
        }

        void HandleClientState(AvahiClient *_client, AvahiClientState _state)
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

        static void BrowseCallback(AvahiServiceBrowser *_browser, AvahiIfIndex _interface, AvahiProtocol _protocol,
                                   AvahiBrowserEvent _event, const char *_name, const char *_type, const char *_domain,
                                   AvahiLookupResultFlags _flags, void *_userData)
        {
            auto *self = static_cast<ServiceDiscovery *>(_userData);
            std::clog << MicaListener::ServiceDiscovery::logName << "Browser Callback: ";
            self->HandleBrowserState(_event, _name, _type, _domain);
        }

        void HandleBrowserState(AvahiBrowserEvent _event, const char *_name, const char *_type, const char *_domain)
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
                    CreateServiceResolver(client.get(), AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, _name, _type, _domain);
                    break;
                case AVAHI_BROWSER_REMOVE:
                    std::clog << "Removed service: " << (_name ? _name : "(null)")
                            << " type: " << (_type ? _type : "(null)")
                            << " domain: " << (_domain ? _domain : "(null)") << std::endl;
                    onServiceLost(_name);
                    break;
                case AVAHI_BROWSER_ALL_FOR_NOW:
                    std::clog << "Avahi Browser has found every entry for now." << std::endl;
                    break;
                case AVAHI_BROWSER_CACHE_EXHAUSTED:
                    std::clog << "Avahi Cache was exhausted." << std::endl;
                    break;
            }
        }

        static void ResolveCallback(AvahiServiceResolver *_resolver, AvahiIfIndex _interface, AvahiProtocol _protocol,
                                    AvahiResolverEvent _event, const char *_name, const char *_type,
                                    const char *_domain, const char *_hostName, const AvahiAddress *_address,
                                    uint16_t _port, AvahiStringList *_text, AvahiLookupResultFlags _flags,
                                    void *_userData)
        {
            const auto *self = static_cast<ServiceDiscovery *>(_userData);
            std::clog << MicaListener::ServiceDiscovery::logName << "Resolve Callback: ";
            self->HandleResolverState(_event, _name, _address, _port);
        }

        void HandleResolverState(AvahiResolverEvent _event, const char *_name, const AvahiAddress *_address,
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
                    std::clog << logName << "Returning info to main..." << std::endl;
                    const NetworkConfig networkConfig(address, _port);
                    onServiceResolved(networkConfig);
                    break;
            }
        }
    };
}
