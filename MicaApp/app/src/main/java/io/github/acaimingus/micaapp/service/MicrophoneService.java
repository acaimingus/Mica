package io.github.acaimingus.micaapp.service;

import android.Manifest;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.app.NotificationCompat;
import java.io.IOException;
import java.util.Arrays;

import io.github.acaimingus.micaapp.R;
import io.github.acaimingus.micaapp.activity.ConnectionCallbacks;
import io.github.acaimingus.micaapp.activity.MainActivity;

public class MicrophoneService extends Service implements IAudioDataListener {

    private RecordingController recordingController;
    private final String notificationChannelId = "MicrophoneServiceChannel";
    private boolean isMuted = false;
    private final IBinder binder = new LocalBinder(this);
    public static boolean isRunning = false;
    private NsdController nsdController;
    private AudioProcessor audioProcessor;
    public ConnectionCallbacks connectionCallbacks;

    @Override
    public void onCreate() {
        super.onCreate();

        // Create the microphone controller
        recordingController = new RecordingController();
        recordingController.setAudioDataListener(this);

        // Create the audio processor
        audioProcessor = new AudioProcessor();

        // Create the NSD controller
        nsdController = new NsdController(this);

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

        nsdController.startNetworkServer();
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
        audioProcessor.gain = value;
        audioProcessor.gainFixed = (int) ((value / 100.0f) * 4096);
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
        nsdController.stopNetworkServer();
        isRunning = false;
    }

    @Override
    public void onAudioDataReceived(byte[] data, int bytesRead) {
        if(nsdController.receiverStream != null) {
            try {
                if (isMuted) {
                    // Send zero array
                    Arrays.fill(data, (byte) 0);
                    nsdController.receiverStream.write(data, 0, bytesRead);
                } else {
                    audioProcessor.applyGain(data, bytesRead);
                    nsdController.receiverStream.write(data, 0, bytesRead);
                }
            } catch (IOException exception) {
                Log.e("MicrophoneService", "Error sending audio data", exception);
                if (connectionCallbacks != null) {
                    connectionCallbacks.onDisconnected();
                }
                nsdController.cleanupClientSocket();
            }
        }
    }
}
