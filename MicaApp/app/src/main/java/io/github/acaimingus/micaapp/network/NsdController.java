package io.github.acaimingus.micaapp.network;

import android.content.Context;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.util.Log;

import java.io.IOException;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;

import io.github.acaimingus.micaapp.service.MicrophoneService;

/**
 * Controller class for managing Android Network Service Discovery (NSD/mDNS) and the
 * TCP server socket that accepts connections from the desktop listener.
 *
 * <p>Registers the microphone service via mDNS so the desktop listener can discover it
 * automatically on the local network, then accepts exactly one TCP connection at a time
 * and exposes an {@link OutputStream} for sending raw audio data to that connection.</p>
 */
public class NsdController {
    /**
     * Android NSD manager used to register and unregister the mDNS service
     */
    private NsdManager nsdManager;

    /**
     * Listener that handles NSD registration callbacks
     */
    private NsdManager.RegistrationListener registrationListener;

    /**
     * TCP server socket that listens for incoming connections from the desktop listener
     */
    private ServerSocket serverSocket;

    /**
     * Background thread that runs the server accept-loop
     */
    private Thread dataSenderThread;

    /**
     * mDNS service type used to advertise and discover this microphone service
     */
    public final String serviceType = "_micaapp._tcp";

    /**
     * Currently connected client socket (desktop listener); {@code null} when no client is connected
     */
    private Socket receiverSocket;

    /**
     * Output stream of the connected client socket; used to write raw audio data to the listener.
     * {@code null} when no client is connected.
     */
    public OutputStream receiverStream;

    private Socket pendingPairingSocket = null;
    private byte[] pendingSecret = null;
    private boolean androidConfirmed = false;
    private boolean pcConfirmed = false;
    private final Object pairingLock = new Object();

    public void confirmPairing() {
        new Thread(() -> {
            synchronized (pairingLock) {
                if (pendingPairingSocket != null) {
                    androidConfirmed = true;
                    try {
                        pendingPairingSocket.getOutputStream().write(0x00);
                        pendingPairingSocket.getOutputStream().flush();
                    } catch (IOException exception) {
                        // Ignore IO exception
                    }
                    checkPairingSuccess();
                }
            }
        }).start();
    }

    public void rejectPairing() {
        new Thread(() -> {
            synchronized (pairingLock) {
                if (pendingPairingSocket != null) {
                    try {
                        pendingPairingSocket.getOutputStream().write(0xFF);
                        pendingPairingSocket.getOutputStream().flush();
                        pendingPairingSocket.close();
                    } catch (IOException exception) {
                        // Ignore IO exception
                    }
                    pendingPairingSocket = null;
                    pendingSecret = null;
                }
            }
        }).start();
    }

    private void checkPairingSuccess() {
        if (androidConfirmed && pcConfirmed && pendingSecret != null) {
            KeystoreManager.saveSharedSecret(microphoneService, pendingSecret);
            Log.i("MicrophoneService", "Pairing confirmed by both sides!");
            try {
                pendingPairingSocket.close();
            } catch (IOException exception) {
                // Ignore IO exception
            }
            pendingPairingSocket = null;
            pendingSecret = null;
        }
    }

    /**
     * Reference to the owning {@link MicrophoneService}, used to invoke connection callbacks
     */
    private final MicrophoneService microphoneService;

    /**
     * Creates a new NsdController bound to the given {@link MicrophoneService}.
     *
     * @param microphoneService the service that owns this controller
     */
    public NsdController(MicrophoneService microphoneService) {
        this.microphoneService = microphoneService;
    }

    /**
     * Starts the TCP server on a random available port and registers it as an mDNS service
     * so the desktop listener can discover it. Runs in a background thread and accepts one
     * client connection at a time.
     */
    public void startNetworkServer() {
        dataSenderThread = new Thread(() -> {
            try {
                serverSocket = new ServerSocket(0);
                int localPort = serverSocket.getLocalPort();
                Log.d("Microphone Service", "Server listens on port: " + localPort);

                initializeRegistrationListener();
                registerService(localPort);

                while (!Thread.currentThread().isInterrupted()) {
                    try {
                        Log.d("MicrophoneService", "Waiting for a PC connection...");
                        Socket clientSocket = serverSocket.accept();
                        clientSocket.setKeepAlive(true);

                        Log.i("MicrophoneService", "PC connected! IP: " + clientSocket.getInetAddress().getHostAddress());

                        clientSocket.setSoTimeout(2000); // 2 second timeout for the handshake
                        java.io.InputStream in = clientSocket.getInputStream();
                        int cmd = in.read();

                        if (cmd == 0x01) {
                            // Pairing Request
                            byte[] peerKey = new byte[32];
                            int read = 0;
                            while (read < 32) {
                                int r = in.read(peerKey, read, 32 - read);
                                if (r == -1) break;
                                read += r;
                            }
                            if (read == 32) {
                                EcdhKeyExchange ecdh = new EcdhKeyExchange();
                                byte[] secret = ecdh.computeSharedSecret(peerKey);
                                if (secret != null) {
                                    String pin = EcdhKeyExchange.derivePinFromSecret(secret);

                                    OutputStream out = clientSocket.getOutputStream();
                                    out.write(0x01);
                                    out.write(ecdh.getPublicKey());
                                    out.flush();

                                    synchronized (pairingLock) {
                                        pendingPairingSocket = clientSocket;
                                        pendingSecret = secret;
                                        androidConfirmed = false;
                                        pcConfirmed = false;
                                    }

                                    // Notify UI
                                    if (microphoneService.connectionCallbacks != null) {
                                        microphoneService.connectionCallbacks.onPairingRequested(pin);
                                    }

                                    // Wait for PC's response, wait infinitely
                                    clientSocket.setSoTimeout(0);
                                    int pcResp = in.read();

                                    synchronized (pairingLock) {
                                        if (pendingPairingSocket == clientSocket) {
                                            if (pcResp == 0x00) {
                                                pcConfirmed = true;
                                                checkPairingSuccess();
                                            } else {
                                                Log.i("MicrophoneService", "PC rejected pairing or disconnected.");
                                                if (microphoneService.connectionCallbacks != null) {
                                                    microphoneService.connectionCallbacks.onPairingRejected();
                                                }
                                                try {
                                                    clientSocket.close();
                                                } catch (IOException exception) {
                                                    // Ignore IO exception
                                                }
                                                pendingPairingSocket = null;
                                                pendingSecret = null;
                                            }
                                        }
                                    }

                                    if (pcResp == 0x00) {
                                        // PC confirmed, but Android user might still be deciding.
                                        // If PC times out and drops the connection, we must dismiss the Android UI.
                                        // We block on another read. If pairing succeeds, the socket is closed 
                                        // by checkPairingSuccess() and this throws an exception (which we catch below).
                                        // If PC disconnects, it returns -1 (or 0xFF).
                                        try {
                                            int next = in.read();
                                            if (next == -1 || next == 0xFF) {
                                                synchronized (pairingLock) {
                                                    if (pendingPairingSocket == clientSocket) {
                                                        Log.i("MicrophoneService", "PC disconnected after initial confirmation.");
                                                        if (microphoneService.connectionCallbacks != null) {
                                                            microphoneService.connectionCallbacks.onPairingRejected();
                                                        }
                                                        try {
                                                            clientSocket.close();
                                                        } catch (IOException exception) {
                                                            // Ignore IO exception
                                                        }
                                                        pendingPairingSocket = null;
                                                        pendingSecret = null;
                                                    }
                                                }
                                            }
                                        } catch (IOException ignored) {
                                        }
                                    }
                                }
                            }
                            // Do not close clientSocket here, it is handled in checkPairingSuccess or reject
                        } else if (cmd == 0x02) {
                            // Stream Request
                            byte[] token = new byte[32];
                            int read = 0;
                            while (read < 32) {
                                int r = in.read(token, read, 32 - read);
                                if (r == -1) break;
                                read += r;
                            }

                            byte[] secret = KeystoreManager.loadSharedSecret(microphoneService);
                            byte[] expectedToken = EcdhKeyExchange.generateAuthToken(secret);

                            boolean valid = (read == 32) && java.util.Arrays.equals(token, expectedToken);

                            if (valid) {
                                clientSocket.setSoTimeout(0); // Reset timeout for infinite stream
                                // Send a callback to the UI, that the connection is there
                                if (microphoneService.connectionCallbacks != null) {
                                    microphoneService.connectionCallbacks.onConnected();
                                }
                                // Clean up the client socket if it was previously initialized
                                cleanupClientSocket();
                                // Set it and get its output stream
                                receiverSocket = clientSocket;
                                receiverStream = receiverSocket.getOutputStream();
                            } else {
                                Log.e("MicrophoneService", "Invalid Auth Token! Closing connection.");
                                clientSocket.close();
                            }
                        } else {
                            Log.e("MicrophoneService", "Unknown command byte: " + cmd);
                            clientSocket.close();
                        }

                    } catch (IOException e) {
                        if (serverSocket == null || serverSocket.isClosed()) {
                            Log.i("MicrophoneService", "ServerSocket closed, stopping thread...");
                            if (microphoneService.connectionCallbacks != null) {
                                microphoneService.connectionCallbacks.onDisconnected();
                            }
                            break;
                        }
                        Log.e("MicrophoneService", "Error on serverSocket.accept()", e);
                    }
                }
            } catch (IOException e) {
                Log.e("MicrophoneService", "Error starting the Server Socket", e);
            }
        });
        dataSenderThread.start();
    }

    /**
     * Stops the network server by unregistering the mDNS service, interrupting the server
     * thread and closing all open sockets.
     */
    public void stopNetworkServer() {
        try {
            if (nsdManager != null && registrationListener != null) {
                nsdManager.unregisterService(registrationListener);
                registrationListener = null;
            }
            cleanupClientSocket();

            if (dataSenderThread != null) {
                dataSenderThread.interrupt();
            }
            if (serverSocket != null) {
                serverSocket.close();
                serverSocket = null;
            }
            Log.i("MicrophoneService", "Network service stopped.");
        } catch (IOException e) {
            Log.e("MicrophoneService", "Error stopping the network service", e);
        }
    }

    /**
     * Creates and assigns the {@link NsdManager.RegistrationListener} that handles mDNS
     * registration events such as successful registration, registration failure and service
     * unregistration.
     */
    public void initializeRegistrationListener() {
        registrationListener = new NsdManager.RegistrationListener() {
            @Override
            public void onServiceRegistered(NsdServiceInfo NsdServiceInfo) {
                String registeredName = NsdServiceInfo.getServiceName();
                Log.d("MicrophoneService", "NSD service registered as: " + registeredName);
            }

            @Override
            public void onRegistrationFailed(NsdServiceInfo serviceInfo, int errorCode) {
                Log.e("MicrophoneService", "NSD service registration failed: " + errorCode);
            }

            @Override
            public void onServiceUnregistered(NsdServiceInfo serviceInfo) {
                Log.i("MicrophoneService", "NSD service deregistered.");
            }

            @Override
            public void onUnregistrationFailed(NsdServiceInfo serviceInfo, int errorCode) {
            }
        };
    }

    /**
     * Registers the microphone service with Android's Network Service Discovery framework
     * so the desktop listener can discover it via mDNS.
     *
     * @param port the local TCP port the server socket is listening on
     */
    public void registerService(int port) {
        NsdServiceInfo serviceInfo = new NsdServiceInfo();
        serviceInfo.setServiceName(DeviceIdentification.getDeviceName(microphoneService));
        serviceInfo.setServiceType(serviceType);
        serviceInfo.setPort(port);

        nsdManager = (NsdManager) microphoneService.getSystemService(Context.NSD_SERVICE);
        if (nsdManager != null) {
            nsdManager.registerService(
                    serviceInfo, NsdManager.PROTOCOL_DNS_SD, registrationListener);
        } else {
            Log.e("MicrophoneService", "NsdManager could not be found.");
        }
    }

    /**
     * Closes and nullifies the current client output stream and client socket, if open.
     * Safe to call when no client is connected.
     */
    public void cleanupClientSocket() {
        if (receiverStream != null) {
            try {
                receiverStream.close();
                receiverStream = null;
            } catch (IOException exception) {
                Log.e("MicrophoneService", "Error closing receiver stream", exception);
            }
        }

        if (receiverSocket != null) {
            try {
                receiverSocket.close();
                receiverSocket = null;
            } catch (IOException exception) {
                Log.e("MicrophoneService", "Error closing receiver socket", exception);
            }
        }
    }
}
