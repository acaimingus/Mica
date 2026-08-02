#include "../Tui/pairingconfirmationtui.cpp"

namespace MicaPairingService::DeviceSelection
{
    class DeviceSelectionHandler
    {
    public:
        static void HandleDeviceSelection()
        {
            Tui::PairingConfirmationTui pairingConfirmationTui;

            std::cout << "Launched the TUI succesfully! yippie!" << std::endl;
            pairingConfirmationTui.ShowPairingConfirmationTui();
        }
    };
}
