package io.github.acaimingus.micaapp;

import android.Manifest;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.app.NotificationCompat;

public class MicrophoneService extends Service {

    private MicrophoneController microphoneController;
    private final int notificationId;
    private final String notificationChannelId;

    public MicrophoneService() {
        notificationId = 1;
        notificationChannelId = "MicrophoneServiceChannel";
    }

    @Override
    public void onCreate() {
        super.onCreate();

        // Create the microphone controller
        microphoneController = new MicrophoneController();

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
                .setContentTitle("Microphone Service")
                .setContentText("Records audio...")
                .setSmallIcon(R.drawable.mic_24px)
                .setOngoing(true)
                .build();

        startForeground(notificationId, notification);

        // START_STICKY = Restart service if killed by OS
        microphoneController.startRecording();
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        microphoneController.stopRecording();
        microphoneController = null;
    }
}
