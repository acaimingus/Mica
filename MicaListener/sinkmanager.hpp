#pragma once

#include <string>
#include <vector>
#include <memory>
#include <array>
#include <iostream>
#include <algorithm>

namespace MicaListener
{
    class SinkManager
    {
    public:
        // Internal technical names
        const std::string sinkName = "Mica-Microphone";
        const std::string sourceName = "Mica-Virtual-Mic";
        
        // Pretty names for applications like discord
        const std::string sinkDescription = "Mica Virtual Sink (Output)";
        const std::string sourceDescription = "Mica Virtual Microphone (Input)";

        SinkManager()
        {
            CheckAndCleanOldDevices();
            CreateDevices();
        }

        ~SinkManager()
        {
            DestroyDevices();
        }

    private:
        static inline const std::string logName = "\033[36mSINKMANAGER\033[0m\t";
        
        // List for all loaded modules (Sink + Remap)
        std::vector<std::string> loadedModuleIds;

        static void CheckAndCleanOldDevices()
        {
            const std::string cmd =
                    "pactl list short modules | grep 'Mica' | cut -f1 | xargs -L1 pactl unload-module 2>/dev/null";

            const int result = system(cmd.c_str());
        }

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

            LoadModule(cmdSource, "Remap-Source (Mic)");
        }

        bool LoadModule(const std::string& cmd, const std::string& debugName)
        {
            std::string id = Execute(cmd);
            
            // Remove the newline at the end
            if (!id.empty() && id.back() == '\n') {
                id.pop_back();
            }

            if (id.empty()) {
                std::cerr << logName << "Failed to load " << debugName << "!" << std::endl;
                return false;
            }

            std::clog << logName << "Loaded " << debugName << " with ID: " << id << std::endl;
            loadedModuleIds.push_back(id);
            return true;
        }

        void DestroyDevices()
        {
            std::cerr << logName << "Cleaning up virtual devices..." << std::endl;
            std::ranges::reverse(loadedModuleIds);

            for (const auto& id : loadedModuleIds) {
                if (!id.empty()) {
                    Execute("pactl unload-module " + id);
                }
            }
            loadedModuleIds.clear();
        }

        static std::string Execute(const std::string &_command)
        {
            std::array<char, 128> buffer{};
            std::string result;
            const std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(_command.c_str(), "r"), pclose);
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
    };
}