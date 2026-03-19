/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Class for creating and deleting the PulseAudio sinks aka the virtual microphone on the computer.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <array>
#include <iostream>
#include <algorithm>
#include <fstream>

namespace MicaListener
{
    class SinkManager
    {
    public:
        /// @brief Internal technical sink name
        const std::string sinkName = "Mica-Microphone";

        /// @brief Internal technical source name
        const std::string sourceName = "Mica-Virtual-Mic";

        /// @brief Pretty sink name (for applications like Discord)
        const std::string sinkDescription = "Mica Virtual Sink (Output)";

        /// @brief Pretty source name (for applications like Discord)
        const std::string sourceDescription = "Mica Virtual Microphone (Input)";

        /// @brief Constructor, checks for old PulseAudio devices, removes them and then creates new devices
        SinkManager()
        {
            CheckAndCleanOldDevices();
            sinkFile.open(".micasinks");
            CreateDevices();
        }

        /// @brief Destructor, cleans up the created PulseAudio devices
        ~SinkManager()
        {
            DestroyDevices();
            sinkFile.close();
            // Program shut down cleanly, doesn't need the ID file anymore
            std::remove(".micasinks");
        }

    private:
        /// @brief Log prefix for the Sink Manager
        static inline const std::string logName = "\033[36mSINKMANAGER\033[0m\t";

        /// @brief List for all loaded modules (Sink + Remap)
        std::vector<std::string> loadedModuleIds;

        /// @brief File containing IDs of all registered PulseAudio devices to clean up in case they weren't for
        /// whatever reason
        std::ofstream sinkFile;

        /// @brief Helper method for loading the module and saving the ID
        /// @param cmd The command to be executed
        /// @param debugName The name of the type of microphone that was attempted to be created
        bool LoadModule(const std::string &cmd, const std::string &debugName)
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

        /// @brief Helper method for executing commands wih popen
        /// @param _command The command to execute
        static std::string Execute(const std::string &_command)
        {
            std::array<char, 128> buffer{};
            std::string result;
            const std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(_command.c_str(), "r"), pclose);
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

        /// @brief Method for finding any remaining virtual devices and removing them
        static void CheckAndCleanOldDevices()
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

        /// @brief Method for creating the needed PulseAudio devices
        void CreateDevices()
        {
            // Create the sink
            const std::string cmdSink = "pactl load-module module-null-sink"
                                        " sink_name=" + sinkName +
                                        " sink_properties=device.description=\"" + sinkDescription + "\"";
            if (!LoadModule(cmdSink, "Null-Sink")) return;

            // Create a microphone out of the sink monitor
            const std::string cmdSource = "pactl load-module module-remap-source"
                                          " master=" + sinkName + ".monitor" +
                                          " source_name=" + sourceName +
                                          " source_properties=device.description=\"" + sourceDescription + "\"";
            if (!LoadModule(cmdSource, "Remap-Source (Mic)")) return;
        }

        /// @brief Method for destroying the remaining PulseAudio devices when quiting
        void DestroyDevices()
        {
            std::cerr << logName << "Cleaning up virtual devices..." << std::endl;
            std::ranges::reverse(loadedModuleIds);

            for (const auto &id: loadedModuleIds)
            {
                if (!id.empty())
                {
                    Execute("pactl unload-module " + id);
                }
            }
            loadedModuleIds.clear();
        }
    };
}
