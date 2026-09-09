/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Class for creating and deleting the PulseAudio sinks aka the virtual microphone on the computer.
 */

#pragma once

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace MicaListener::MicaListenerService::Audio
{
class SinkManager
{
  public:
    /// @brief Constructor, checks for old PulseAudio devices, removes them and then creates new devices
    SinkManager();

    /// @brief Destructor, cleans up the created PulseAudio devices
    ~SinkManager();

  private:
    /// @brief Log prefix for the Sink Manager
    static inline const std::string logName = "\033[36mSINKMANAGER\033[0m\t";

    /// @brief Internal technical sink name
    const std::string sinkName = "Mica-Microphone";

    /// @brief Internal technical source name
    const std::string sourceName = "Mica-Virtual-Mic";

    /// @brief Pretty sink name (for applications like Discord)
    const std::string sinkDescription = "Mica Virtual Sink (Output)";

    /// @brief Pretty source name (for applications like Discord)
    const std::string sourceDescription = "Mica Virtual Microphone (Input)";

    /// @brief List for all loaded modules (Sink + Remap)
    std::vector<std::string> loadedModuleIds;

    /// @brief File containing IDs of all registered PulseAudio devices to clean up in case they weren't for
    /// whatever reason
    std::ofstream sinkFile;

    /// @brief Helper method for loading the module and saving the ID
    /// @param cmd The command to be executed
    /// @param debugName The name of the type of microphone that was attempted to be created
    bool LoadModule(const std::string &cmd, const std::string &debugName);

    /// @brief Helper method for executing commands wih popen
    /// @param _command The command to execute
    static std::string Execute(const std::string &_command);

    /// @brief Method for finding any remaining virtual devices and removing them
    static void CheckAndCleanOldDevices();

    /// @brief Method for creating the needed PulseAudio devices
    void CreateDevices();

    /// @brief Method for destroying the remaining PulseAudio devices when quiting
    void DestroyDevices();
};
} // namespace MicaListener::MicaListenerService::Audio
