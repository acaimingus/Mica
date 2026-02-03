#pragma once

#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>

namespace MicaListener
{
    class AudioPlayer
    {
    public:
        /// @brief Destructor for the Audioplayer
        ~AudioPlayer();

        /// @brief Initialization method, creates the OpenAL devices, context, audio sources and buffers
        /// @param _deviceName The name of the device
        void Initialize(const std::string &_deviceName);

        /// @brief Method for playback of the received buffer data
        /// @param _buffer The buffer to be played
        void PlayBuffer(const std::vector<uint8_t> &_buffer);

        /// @brief Helper method for getting the available OpenAL devices
        static void GetAvailableDevices();

    private:
        /// @brief Component name in the logger
        static const std::string logName;

        /// @brief Deleter struct for freeing the memory of the OpenAL device using the C method
        struct ALCDeviceDeleter
        {
            void operator()(ALCdevice *_device) const;
        };

        /// @brief Deleter struct for freeing the memory of the OpenAL context using the C method
        struct ALCcontextDeleter
        {
            void operator()(ALCcontext *_context) const;
        };

        /// @brief Smart pointer for the OpenAL device
        std::unique_ptr<ALCdevice, ALCDeviceDeleter> device;

        /// @brief Smart pointer for the OpenAL context
        std::unique_ptr<ALCcontext, ALCcontextDeleter> context;

        /// @brief OpenAL audio source
        ALuint audioSource = 0;

        /// @brief Many small buffers for smooth audio playback
        std::vector<ALuint> audioBuffers;

        /// @brief Queue for free audio buffers
        std::queue<ALuint> freeBuffers;
    };
}