#include "audioplayer.hpp"

namespace MicaListener
{
    const std::string AudioPlayer::logName = "\033[35mAUDIOPLAYER\033[0m\t";

    AudioPlayer::~AudioPlayer()
    {
        // Clean up the audio resources
        if (audioSource)
        {
            alSourceStop(audioSource);
            alDeleteSources(1, &audioSource);
        }

        // Clean up the audio buffers behind the smart pointer
        if (!audioBuffers.empty())
        {
            alDeleteBuffers(audioBuffers.size(), audioBuffers.data());
        }
    }

    void AudioPlayer::Initialize(const std::string &_deviceName)
    {
        // Get the device
        device.reset(alcOpenDevice(_deviceName.empty() ? nullptr : _deviceName.c_str()));
        // Check if device opened correctly
        if (!device)
        {
            std::cerr << logName << "Failed to open OpenAL device!" << std::endl;
            return;
        }

        // Create the context
        context.reset(alcCreateContext(device.get(), nullptr));
        // Check if context created correctly
        if (!context)
        {
            std::cerr << logName << "Failed to create OpenAL context!" << std::endl;
            return;
        }

        // Make the context current
        if (!alcMakeContextCurrent(context.get()))
        {
            std::cerr << logName << "Failed to make OpenAL context current!" << std::endl;
            return;
        }

        // Create an audio source
        alGenSources(1, &audioSource);
        // Check if the audio source was created correctly
        ALenum error = alGetError();
        if (error != AL_NO_ERROR)
        {
            std::cerr << logName << "Failed to create OpenAL audio source! Error code: " << error << std::endl;
            return;
        }

        // Create many small audio buffers for smooth playback
        audioBuffers.resize(16);
        alGenBuffers(16, audioBuffers.data());
        // Check if the buffers created successfully
        error = alGetError();
        if (error != AL_NO_ERROR)
        {
            std::cerr << logName << "Failed to create OpenAL audio buffers! Error code: " << error << std::endl;
            return;
        }
        // Add all buffers to the free queue
        for (ALuint buffer: audioBuffers)
        {
            freeBuffers.push(buffer);
        }
    }

    void AudioPlayer::PlayBuffer(const std::vector<uint8_t> &_buffer)
    {
        // Abort if the buffer is empty
        if (_buffer.empty())
        {
            return;
        }

        // Check if a buffer finished playing
        ALint doneBuffers;
        alGetSourcei(audioSource, AL_BUFFERS_PROCESSED, &doneBuffers);
        while (doneBuffers > 0)
        {
            ALuint doneBufferId;
            // Remove the finished buffer from the queue
            alSourceUnqueueBuffers(audioSource, 1, &doneBufferId);
            // Add the finished buffer back to the free queue
            freeBuffers.push(doneBufferId);
            // Decrease the done buffer count
            doneBuffers--;
        }

        // Check if all buffers filled up
        if (freeBuffers.empty())
        {
            // There are no free buffers, skip a packet to catch up
            std::cerr << logName << "BUFFER UNDERRUN! Dropping audio packets to catch up." << std::endl;
            return;
        }

        // Get the ID of a free buffer
        const ALuint bufferId = freeBuffers.front();
        freeBuffers.pop();
        // Push the data to OpenAL
        alBufferData(bufferId, AL_FORMAT_MONO16, _buffer.data(), _buffer.size(), 44100);

        // Check if it worked
        if (const auto error = alGetError(); error != AL_NO_ERROR)
        {
            // It did not work :(
            std::cerr << logName << "Error uploading buffer data: " << error << std::endl;
            // Free the buffer again and exit
            freeBuffers.push(bufferId);
            return;
        }

        // Queue the buffer to be played in OpenAL
        alSourceQueueBuffers(audioSource, 1, &bufferId);

        // Make sure the source is always playing
        ALint state;
        alGetSourcei(audioSource, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING)
        {
            alSourcePlay(audioSource);
        }
    }

    void AudioPlayer::GetAvailableDevices()
    {
        if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT"))
        {
            const ALCchar *devices = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
            const ALCchar *current = devices;

            // Print the available audio devices
            std::cout << logName << "Available Audio Devices:" << std::endl;
            while (current && *current != '\0')
            {
                std::cout << "\t- " << current << std::endl;
                std::string_view sv(current);
                current += sv.length() + 1;
            }
        }
    }

    void AudioPlayer::ALCDeviceDeleter::operator()(ALCdevice *_device) const
    {
        if (_device)
        {
            alcCloseDevice(_device);
        }
    }

    void AudioPlayer::ALCcontextDeleter::operator()(ALCcontext *_context) const
    {
        if (_context)
        {
            alcDestroyContext(_context);
        }
    }
}
