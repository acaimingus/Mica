package io.github.acaimingus.micaapp.service;

import android.Manifest;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.os.IBinder;
import android.util.Log;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.app.NotificationCompat;
import java.io.IOException;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;

import io.github.acaimingus.micaapp.R;

public class MicrophoneService extends Service implements IAudioDataListener {

    private RecordingController recordingController;
    private final String notificationChannelId = "MicrophoneServiceChannel";
    private NsdManager nsdManager;
    private NsdManager.RegistrationListener registrationListener;
    private ServerSocket serverSocket;
    private Thread dataSenderThread;
    private Socket receiverSocket;
    private OutputStream receiverStream;
    public final String serviceType = "_micaapp._tcp";

    @Override
    public void onCreate() {
        super.onCreate();

        // Create the microphone controller
        recordingController = new RecordingController();

        recordingController.setAudioDataListener(this);

        // Create a notification channel
        NotificationChannel serviceChannel = new NotificationChannel(
                notificationChannelId,
                "Microphone Service Channel",
                NotificationManager.IMPORTANCE_LOW
        );
        NotificationManager manager = getSystemService(NotificationManager.class);
        if (manager != null) {
            manager.createNotificationChannel(serviceChannel);
        }

        startNetworkServer();
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @RequiresPermission(Manifest.permission.RECORD_AUDIO)
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Notification notification = new NotificationCompat.Builder(this, notificationChannelId)
                .setContentTitle("Mica Microphone")
                .setContentText("Currently connected...")
                .setSmallIcon(R.drawable.mic_24px)
                .setOngoing(true)
                .build();

        startForeground(1, notification);

        // START_STICKY = Restart service if killed by OS
        recordingController.startRecording();
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        recordingController.stopRecording();
        recordingController = null;
        stopNetworkServer();
    }

    private void startNetworkServer() {
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

                        cleanupClientSocket();

                        receiverSocket = clientSocket;
                        receiverStream = receiverSocket.getOutputStream();

                    } catch (IOException e) {
                        if (serverSocket == null || serverSocket.isClosed()) {
                            Log.i("MicrophoneService", "ServerSocket closed, stopping thread...");
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

    private void stopNetworkServer() {
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

        nsdManager = (NsdManager) getSystemService(Context.NSD_SERVICE);
        if (nsdManager != null) {
            nsdManager.registerService(
                    serviceInfo, NsdManager.PROTOCOL_DNS_SD, registrationListener);
        } else {
            Log.e("MicrophoneService", "NsdManager could not be found.");
        }
    }

    @Override
    public void onAudioDataReceived(byte[] data, int bytesRead) {
        if(receiverStream != null) {
            try {
                receiverStream.write(data, 0, bytesRead);
            } catch (IOException exception) {
                Log.e("MicrophoneService", "Error sending audio data", exception);
                cleanupClientSocket();
            }
        }
    }

    private void cleanupClientSocket() {
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
