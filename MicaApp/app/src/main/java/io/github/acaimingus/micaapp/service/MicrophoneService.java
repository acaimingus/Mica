package io.github.acaimingus.micaapp.service;

import android.Manifest;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.app.NotificationCompat;
import java.io.IOException;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Arrays;

import io.github.acaimingus.micaapp.R;
import io.github.acaimingus.micaapp.activity.ConnectionCallbacks;
import io.github.acaimingus.micaapp.activity.MainActivity;

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
    private boolean isMuted = false;
    private int gain = 100;
    private int gainFixed = 4096;
    private final IBinder binder = new LocalBinder(this);
    public static boolean isRunning = false;

    private ConnectionCallbacks connectionCallbacks;

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
        return binder;
    }

    public void setConnectionCallbacks(ConnectionCallbacks callbacks) {
        connectionCallbacks = callbacks;
    }

    public void setIsMuted(boolean value) {
        isMuted = value;
    }

    public boolean getIsMuted() {
        return isMuted;
    }

    public void setGain(int value) {
        gain = value;
        this.gainFixed = (int) ((value / 100.0f) * 4096);
    }

    @RequiresPermission(Manifest.permission.RECORD_AUDIO)
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // Create an intent to open the app when clicking on the notífication
        PendingIntent notificationIntent = PendingIntent.getActivity(this, 0, new Intent(this, MainActivity.class), PendingIntent.FLAG_IMMUTABLE);

        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, notificationChannelId)
                .setContentTitle("Mica Microphone")
                .setContentText("Currently connected...")
                .setSmallIcon(R.drawable.mic_24px)
                .setContentIntent(notificationIntent)
                .setOngoing(true);

        // Fix: Show the notification immediately, don't delay it
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            builder.setForegroundServiceBehavior(Notification.FOREGROUND_SERVICE_IMMEDIATE);
        }

        Notification notification = builder.build();

        // Justify the service type
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.Q) {
            startForeground(1, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MICROPHONE);
        } else {
            startForeground(1, notification);
        }

        recordingController.startRecording();

        isRunning = true;

        // START_STICKY = Restart service if killed by OS
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        recordingController.stopRecording();
        recordingController = null;
        stopNetworkServer();
        isRunning = false;
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

                        // Send a callback to the UI, that the connection is there
                        if (connectionCallbacks != null) {
                            connectionCallbacks.onConnected();
                        }

                        // Clean up the client socket  if it was previously initialized
                        cleanupClientSocket();
                        // Set it and get its output stream
                        receiverSocket = clientSocket;
                        receiverStream = receiverSocket.getOutputStream();

                    } catch (IOException e) {
                        if (serverSocket == null || serverSocket.isClosed()) {
                            Log.i("MicrophoneService", "ServerSocket closed, stopping thread...");
                            if (connectionCallbacks != null) {
                                connectionCallbacks.onDisconnected();
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
                if (isMuted) {
                    // Send zero array
                    Arrays.fill(data, (byte) 0);
                    receiverStream.write(data, 0, bytesRead);
                } else {
                    applyGain(data, bytesRead);
                    receiverStream.write(data, 0, bytesRead);
                }
            } catch (IOException exception) {
                Log.e("MicrophoneService", "Error sending audio data", exception);
                if (connectionCallbacks != null) {
                    connectionCallbacks.onDisconnected();
                }
                cleanupClientSocket();
            }
        }
    }

    private void applyGain(byte[] data, int bytesRead) {
        // No gain to apply
        if (gain == 100) {
            return;
        }

        // 2 steps because we are working with 16 bit
        for (int i = 0; i < bytesRead; i+= 2) {
            // Combine 2 bytes to a 16 bit integer
            int audioSample = (data[i] & 0xFF) | (data[i+1] << 8);
            // Apply gain, apply our factor and divide by 4096
            int newSample = (audioSample * gainFixed) >> 12;
            // Prevent clipping by bounding the values
            if (newSample > 32767) {
                newSample = 32767;
            } else if (newSample < -32768) {
                newSample = -32768;
            }
            // Put the result back in the array
            data[i] = (byte) newSample;
            data[i+1] = (byte) (newSample >> 8);
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
