#include "../Tui/deviceselectiontui.cpp"

namespace MicaPairingService::DeviceSelection
{
    class DeviceSelectionHandler
    {
    public:
        static void HandleDeviceSelection()
        {
            Tui::DeviceSelectionTui deviceSelectionTui;

            std::cout << "Launched the TUI succesfully! yippie!" << std::endl;
            deviceSelectionTui.ShowDeviceSelectionTui();
        }
    };
}
