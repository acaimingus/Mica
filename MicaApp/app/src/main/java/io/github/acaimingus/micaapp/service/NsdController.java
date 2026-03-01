package io.github.acaimingus.micaapp.service;

import android.content.Context;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.util.Log;

import java.io.IOException;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;

public class NsdController {
    private NsdManager nsdManager;
    private NsdManager.RegistrationListener registrationListener;
    private ServerSocket serverSocket;
    private Thread dataSenderThread;
    public final String serviceType = "_micaapp._tcp";
    private Socket receiverSocket;
    public OutputStream receiverStream;
    private final MicrophoneService microphoneService;

    public NsdController(MicrophoneService microphoneService) {
        this.microphoneService = microphoneService;
    }

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

                        Log.i("MicrophoneService", "PC connected! IP: " + clientSocket.getInetAddress().getHostAddress());

                        // Send a callback to the UI, that the connection is there
                        if (microphoneService.connectionCallbacks != null) {
                            microphoneService.connectionCallbacks.onConnected();
                        }

                        // Clean up the client socket  if it was previously initialized
                        cleanupClientSocket();
                        // Set it and get its output stream
                        receiverSocket = clientSocket;
                        receiverStream = receiverSocket.getOutputStream();

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

    public void initializeRegistrationListener() {
        registrationListener = new NsdManager.RegistrationListener() {
            @Override
            public void onServiceRegistered(NsdServiceInfo NsdServiceInfo) {
                String registeredName = NsdServiceInfo.getServiceName();
                Log.d("MicrophoneService", "NSD service registered as: " + registeredName);
            }
            @Override public void onRegistrationFailed(NsdServiceInfo serviceInfo, int errorCode) {
                Log.e("MicrophoneService", "NSD service registration failed: " + errorCode);
            }
            @Override public void onServiceUnregistered(NsdServiceInfo serviceInfo) {
                Log.i("MicrophoneService", "NSD service deregistered.");
            }
            @Override public void onUnregistrationFailed(NsdServiceInfo serviceInfo, int errorCode) { }
        };
    }

    public void registerService(int port) {
        NsdServiceInfo serviceInfo = new NsdServiceInfo();
        serviceInfo.setServiceName("MicaAppMicrophoneService");
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
