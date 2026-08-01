#pragma once

#include "../Tui/deviceselectiontui.cpp"

namespace MicaPairingService::DeviceSelection
{
    class DeviceSelectionHandler
    {
    public:
        static void HandleDeviceSelection()
        {
            Tui::DeviceSelectionTui deviceSelectionTui{};
            deviceSelectionTui.ShowDeviceSelectionTui();
        }
    };
}
