/*
 * Copyright (c) 2026 Adam Martula
 * This source code is licensed under the MIT license found in the LICENSE file in the root of this source tree.
 *
 * Description: Terminal launcher helper to ensure MicaPairingService runs inside a visible terminal emulator.
 */

#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

namespace MicaPairingService::Terminal
{
    class TerminalLauncher
    {
    public:
        /// @brief Ensures the process is running in an interactive TTY window.
        ///        If not, spawns an available terminal emulator running this executable and exits the background parent.
        static void EnsureTerminalWindow(const int argc, char *argv[])
        {
            // If already attached to a terminal, no action needed
            if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO))
            {
                return;
            }

            const std::string exePath = std::filesystem::canonical(argv[0]).string();

            std::vector<std::string> deviceArgs;
            for (int i = 1; i < argc; ++i)
            {
                deviceArgs.emplace_back(argv[i]);
            }

            const std::vector<std::pair<std::string, std::string>> candidates = {
                {"/usr/bin/gnome-terminal", "--"},
                {"/usr/bin/x-terminal-emulator", "-e"},
                {"/usr/bin/konsole", "-e"},
                {"/usr/bin/xterm", "-e"},
                {"/usr/bin/ptyxis", "--"},
                {"/usr/bin/alacritty", "-e"},
                {"/usr/bin/kitty", "-e"}
            };

            for (const auto &[term, flag] : candidates)
            {
                if (std::filesystem::exists(term))
                {
                    pid_t pid = fork();
                    if (pid == 0)
                    {
                        std::vector<std::string> cmdStrings = {term, flag, exePath};
                        for (const auto &arg : deviceArgs)
                        {
                            cmdStrings.push_back(arg);
                        }

                        std::vector<char *> cArgs;
                        for (auto &s : cmdStrings)
                        {
                            cArgs.push_back(s.data());
                        }
                        cArgs.push_back(nullptr);

                        execv(term.c_str(), cArgs.data());
                        _exit(1);
                    }
                    else if (pid > 0)
                    {
                        // Background parent process exits immediately after spawning the terminal window
                        exit(0);
                    }
                }
            }
        }
    };
}
