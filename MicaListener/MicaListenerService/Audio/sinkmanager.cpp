/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Class for creating and deleting the PulseAudio sinks aka the virtual microphone on the computer.
 */

#include "sinkmanager.hpp"

namespace MicaListener::MicaListenerService::Audio
{
SinkManager::SinkManager()
{
    std::clog << logName << "Creating sink device..." << std::endl;
    CheckAndCleanOldDevices();
    sinkFile.open(".micasinks");
    CreateDevices();
    std::clog << logName << "Sink device created successfully!" << std::endl;

    setenv("PULSE_SINK", sinkName.c_str(), 1);
    std::clog << logName << "Enforced PulseAudio Sink: " << sinkName << std::endl;
}

SinkManager::~SinkManager()
{
    DestroyDevices();
    sinkFile.close();
    // Program shut down cleanly, doesn't need the ID file anymore
    std::remove(".micasinks");
}

bool SinkManager::LoadModule(const std::string &cmd, const std::string &debugName)
{
    std::string id = Execute(cmd);

    // Remove the newline at the end
    if (!id.empty() && id.back() == '\n')
    {
        id.pop_back();
    }

    if (id.empty())
    {
        std::cerr << logName << "Failed to load " << debugName << "!" << std::endl;
        return false;
    }

    std::clog << logName << "Loaded " << debugName << " with ID: " << id << std::endl;
    // Save the ID to a vector for later removal at a clean shutdown of the program
    loadedModuleIds.push_back(id);
    // Save the ID to a local file on disk for when the app might fail to clean up a module after a crash
    if (sinkFile.is_open())
    {
        sinkFile << id << std::endl;
    }
    return true;
}

std::string SinkManager::Execute(const std::string &_command)
{
    std::array<char, 128> buffer{};
    std::string result;
    const std::unique_ptr<FILE, int (*)(FILE *)> pipe(popen(_command.c_str(), "r"), pclose);
    if (!pipe)
    {
        std::cerr << "popen() failed!" << std::endl;
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}

void SinkManager::CheckAndCleanOldDevices()
{
    // Get the
    std::ifstream moduleFile(".micasinks");
    std::string oldId;
    while (std::getline(moduleFile, oldId))
    {
        if (!oldId.empty())
        {
            std::cerr << logName << "Cleaning up left-over ID from previous run: " << oldId << std::endl;
            Execute("pactl unload-module " + oldId);
        }
    }
    moduleFile.close();
}

void SinkManager::CreateDevices()
{
    // Create the sink
    const std::string cmdSink = "pactl load-module module-null-sink"
                                " sink_name=" +
                                sinkName + " sink_properties=device.description=\"" + sinkDescription + "\"";
    if (!LoadModule(cmdSink, "Null-Sink"))
        return;

    // Create a microphone out of the sink monitor
    const std::string cmdSource = "pactl load-module module-remap-source"
                                  " master=" +
                                  sinkName + ".monitor" + " source_name=" + sourceName +
                                  " source_properties=device.description=\"" + sourceDescription + "\"";
    if (!LoadModule(cmdSource, "Remap-Source (Mic)"))
        return;
}

void SinkManager::DestroyDevices()
{
    std::cerr << logName << "Cleaning up virtual devices..." << std::endl;
    std::ranges::reverse(loadedModuleIds);

    for (const auto &id : loadedModuleIds)
    {
        if (!id.empty())
        {
            Execute("pactl unload-module " + id);
        }
    }
    loadedModuleIds.clear();
}
} // namespace MicaListener::MicaListenerService::Audio
