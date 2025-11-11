import javax.jmdns.ServiceEvent;
import javax.jmdns.ServiceInfo;
import javax.jmdns.ServiceListener;
import java.io.IOException;
import java.io.InputStream;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.Socket;

public class MicaAppListener implements ServiceListener {

    public final String serviceType = "_micaapp._tcp.local.";
    private Socket socket = null;

    @Override
    public void serviceAdded(ServiceEvent serviceEvent) {
        // Log output
        System.out.println("Service found: " + serviceEvent.getName());

        // Request DNS resolution
        // This is the initial resolution to check if the app service is even available (this is why the timeout is only 1ms)
        serviceEvent.getDNS().requestServiceInfo(serviceEvent.getType(), serviceEvent.getName(), 1);
    }

    @Override
    public void serviceRemoved(ServiceEvent serviceEvent) {
        // Log output
        System.out.println("Service removed: " + serviceEvent.getName());

        // Clean up the socket
        if (socket != null) {
            try {
                socket.close();
            } catch (IOException exception) {
                System.err.println("Error closing the socket: " + exception.getMessage());
            }
            socket = null;
        }
    }

    @Override
    public void serviceResolved(ServiceEvent serviceEvent) {
        // Get the information of the service
        ServiceInfo information = serviceEvent.getInfo();

        // Get the port from the information
        int servicePort = information.getPort();

        // IP address for the service, needs to be determined
        InetAddress ipAddress;

        // Get all available IPv4 addresses
        Inet4Address[] ipv4Addresses = information.getInet4Addresses();
        if (ipv4Addresses.length > 0) {
            // If an IPv4 address is available, set it as the IP
            ipAddress = ipv4Addresses[0];
        } else {
            // Log an error
            System.err.println("No IPv4 addresses for " + serviceEvent.getName() + " available! Trying IPv6...");

            // Get all available IPv6 addresses
            Inet6Address[] ipv6Addresses = information.getInet6Addresses();
            if (ipv6Addresses.length > 0)
            {
                // If an IPv6 address is available, set it as the IP
                ipAddress = ipv6Addresses[0];
            } else {
                // There are no IP addresses available for the service, cancel resolution
                System.err.println("No IPv4 addresses for \" + serviceEvent.getName() + \" available! Cancelling resolution...");
                return;
            }
        }

        // Output the resolved config
        System.out.println("Service was resolved! " + serviceEvent.getName() + " / " + ipAddress.getHostAddress() + " / " + servicePort);

        // Connect to the socket
        connectToSocket(ipAddress, servicePort);
    }

    private void connectToSocket(InetAddress ip, int port)
    {
        if (socket == null || socket.isClosed()) {
            try {
                // Log to console
                System.out.println("Connecting to the service...");

                // Create the socket
                socket = new Socket(ip, port);

                // Start reading data
                readSocketData(socket);

            } catch (IOException exception) {
                // Log error
                System.err.println("Connecting to the socket failed! " + exception.getMessage());
            }
        }
    }

    private void readSocketData(Socket socket) {
        Thread dataReceiverThread = new Thread(() -> {
            InputStream input = null;
            try {
                System.out.println("Data Receiver Thread started! Waiting for data...");

                // Create a reader for the socket
                input = socket.getInputStream();
                byte[] buffer = new byte[4096];
                int bytesRead;

                // Continously read data from the socket
                while ((bytesRead = input.read(buffer)) != -1) {
                    long sum = 0;
                    for (int i = 0; i < bytesRead; i++) {
                        sum += Math.abs(buffer[i]);
                    }
                    double average = (double) sum / bytesRead;
                    System.out.println("Audio data read: " + average);
                }


                // Connection ended for some reason
                System.out.println("The connection has been closed...");
            } catch (IOException exception) {
                if (!socket.isClosed()) {
                    System.err.println("Error in the receiver thread! " + exception.getMessage());
                } else {
                    System.out.println("Data receiver Thread has stopped!");
                }
            } finally {
                // Clean up opened resources
                try {
                    // Close the reader
                    if (input != null) {
                        input.close();
                    }

                    // Close the socket
                    if(socket != null) {
                        socket.close();
                    }
                } catch (IOException exception) {
                    System.err.println("Error closing receiver thread! " + exception.getMessage());
                }
            }
        });
        dataReceiverThread.start();
    }
}
