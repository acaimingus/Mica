#include "../Tui/pairingconfirmationtui.cpp"

namespace MicaPairingService::DeviceSelection
{
    class DeviceSelectionHandler
    {
    public:
        static void HandleDeviceSelection()
        {
            Tui::PairingConfirmationTui pairingConfirmationTui;

            /* TODO: Determine if a GUI can be displayed, if not then use TUI as a back up
             * If all fails, show a console message to manually open the selection
             */

            /* TODO: Determine if there are multiple devices or a single device available here
             * If only one is available, then immediately show the pairing confirmation
             * If more than one is available, then first show the device selection
             */

            std::cout << "=== MicaPairingService TUI ===" << std::endl;
            pairingConfirmationTui.ShowPairingConfirmationTui();
        }
    };
}
