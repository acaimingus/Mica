import javax.jmdns.JmDNS;
import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.NoSuchElementException;
import java.util.Objects;
import java.util.Scanner;

public class Launcher {
    public static void main(String[] args) {
        // Log the start
        System.out.println("Trying to start the MicaListener...");

        // Try to get the localhost IP address
        InetAddress wifiInterfaceIp = null;
        try {
            // Set localhost as the address
            wifiInterfaceIp = InetAddress.getLocalHost();
        } catch (UnknownHostException e) {
            // Print an error message
            System.err.println("Failed getting localhost! Exiting...");
            // Exit with a signal for an error
            System.exit(1);
        }

        try (JmDNS jmDNS = JmDNS.create(wifiInterfaceIp)) {
            MicaAppListener micaAppListener = new MicaAppListener();
            jmDNS.addServiceListener(micaAppListener.serviceType, micaAppListener);

            // Service loop
            System.out.println("MicaListener started! Type 'exit' to exit...");
            boolean exitCondition = false;
            Scanner scanner = new Scanner(System.in);
            while (!exitCondition) {
                try {
                    String input = scanner.nextLine();
                    if (Objects.equals(input, "exit")) {
                        exitCondition = true;
                    }
                } catch (NoSuchElementException IllegalStateException)
                {
                    // The user put in something stupid like the end of transmission character, show the error and exit
                    System.err.println("Input ended unexpectedly! Exiting...");
                    System.exit(1);
                }
            }

            // Cleanup and shutdown
            System.out.println("Shutting Listener Service down...");
            jmDNS.removeServiceListener(micaAppListener.serviceType, micaAppListener);
        } catch (IOException exception) {
            // Print an error message
            System.err.println("Failed to create JmDNS instance! Exiting...");
            // Exit with a signal for an error
            System.exit(1);
        }
    }
}
