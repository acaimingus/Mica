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
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.Log;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.app.NotificationCompat;
import java.io.IOException;
import java.util.Arrays;

import io.github.acaimingus.micaapp.R;
import io.github.acaimingus.micaapp.network.ConnectionCallbacks;
import io.github.acaimingus.micaapp.activity.MainActivity;
import io.github.acaimingus.micaapp.audio.AudioProcessor;
import io.github.acaimingus.micaapp.audio.IAudioDataListener;
import io.github.acaimingus.micaapp.audio.RecordingController;
import io.github.acaimingus.micaapp.network.NsdController;

/**
 * Foreground service responsible for recording audio from the microphone and transmitting
 * it to the connected desktop listener over the network.
 *
 * <p>The service creates a TCP server via {@link NsdController},
 * advertises itself over mDNS and continuously streams raw 16-bit PCM audio to any connected
 * client. Volume control and mute functionality are supported via
 * {@link io.github.acaimingus.micaapp.audio.AudioProcessor}.</p>
 */
public class MicrophoneService extends Service implements IAudioDataListener {

    /**
     * Controller for managing the microphone recording
     */
    private RecordingController recordingController;

    /**
     * Identifier for the notification channel used by this foreground service
     */
    private final String notificationChannelId = "MicrophoneServiceChannel";

    /**
     * Whether the microphone is currently muted; muted audio is replaced with silence
     */
    private boolean isMuted = false;

    /**
     * Binder returned to clients that bind to this service
     */
    private final IBinder binder = new LocalBinder(this);

    /**
     * {@code true} while the service is running as a foreground service; used by the UI to
     * reflect the current connection state
     */
    public static boolean isRunning = false;

    /**
     * Controller for network service discovery and the TCP server socket
     */
    private NsdController nsdController;

    /**
     * Processor for applying gain adjustments to recorded audio data
     */
    private AudioProcessor audioProcessor;

    /**
     * Optional callback interface used to notify the UI of connection state changes and
     * network statistics
     */
    public ConnectionCallbacks connectionCallbacks;

    /**
     * Running total of bytes sent to the connected listener since the service started
     */
    private long totalBytesSent = 0;

    /**
     * Number of bytes sent in the current one-second interval; reset every second by
     * {@link #statsRunnable}
     */
    private int bytesInCurrentSecond = 0;

    /**
     * Handler that runs {@link #statsRunnable} on the main thread once per second
     */
    private Handler statsHandler;

    /**
     * Runnable that reports network statistics to {@link #connectionCallbacks} once per second
     * and resets the per-second byte counter
     */
    private Runnable statsRunnable;

    /**
     * Called by the system when the service is first created.
     * Initializes the recording controller, audio processor and NSD controller,
     * creates the notification channel and starts the network server.
     */
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

        // Runnable for the timer for the statistics
        statsHandler = new Handler(Looper.getMainLooper());
        statsRunnable = new Runnable() {
            @Override
            public void run() {
                if (connectionCallbacks != null && isRunning) {
                    connectionCallbacks.onNetworkStatsUpdated(totalBytesSent, bytesInCurrentSecond);
                }
                // Reset the rate
                bytesInCurrentSecond = 0;

                // Call yourself again in 1 second
                statsHandler.postDelayed(this, 1000);
            }
        };

        nsdController.startNetworkServer();
    }

    /**
     * Returns the {@link LocalBinder} so clients can obtain a reference to this service.
     *
     * @param intent the Intent that was used to bind to the service
     * @return the {@link IBinder} through which clients interact with the service
     */
    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    /**
     * Sets the callback interface that is notified about connection state changes and
     * network statistics.
     *
     * @param callbacks the {@link ConnectionCallbacks} to receive events, or {@code null} to
     *                  remove the current callback
     */
    public void setConnectionCallbacks(ConnectionCallbacks callbacks) {
        connectionCallbacks = callbacks;
    }

    /**
     * Sets whether the microphone is muted. When muted, audio data is replaced with silence
     * before being sent to the listener.
     *
     * @param value {@code true} to mute, {@code false} to unmute
     */
    public void setIsMuted(boolean value) {
        isMuted = value;
    }

    /**
     * Returns whether the microphone is currently muted.
     *
     * @return {@code true} if muted, {@code false} otherwise
     */
    public boolean getIsMuted() {
        return isMuted;
    }

    /**
     * Sets the audio gain. Also updates the fixed-point gain factor used for efficient
     * integer-based gain calculation in {@link AudioProcessor}.
     *
     * @param value gain percentage (100 = no change, 200 = double the volume)
     */
    public void setGain(int value) {
        audioProcessor.gain = value;
        audioProcessor.gainFixed = (int) ((value / 100.0f) * 4096);
    }

    /**
     * Called by the system each time the service is started with
     * {@link android.content.Context#startForegroundService}. Promotes the service to a
     * foreground service by showing a persistent notification, resets the statistics counters,
     * starts the statistics timer and begins microphone recording.
     *
     * @param intent  the Intent supplied to {@code startService}; may be {@code null}
     * @param flags   additional data about the start request
     * @param startId unique integer representing this specific start request
     * @return {@link #START_STICKY} so the service is restarted if killed by the OS
     */
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

        // Set the initial values for the statistics
        totalBytesSent = 0;
        bytesInCurrentSecond = 0;
        // Start the runnable
        statsHandler.post(statsRunnable);

        recordingController.startRecording();
        isRunning = true;

        // START_STICKY = Restart service if killed by OS
        return START_STICKY;
    }

    /**
     * Called by the system when the service is being destroyed. Stops microphone recording
     * and shuts down the network server.
     */
    @Override
    public void onDestroy() {
        super.onDestroy();
        recordingController.stopRecording();
        recordingController = null;
        nsdController.stopNetworkServer();
        isRunning = false;
    }

    /**
     * Called by {@link io.github.acaimingus.micaapp.audio.RecordingController} whenever
     * a new chunk of microphone data is available.
     * If muted, writes silence to the network stream instead of real audio.
     * Otherwise, applies the current gain and writes the data to the network stream.
     * Triggers a disconnection callback on I/O errors.
     *
     * @param data      byte array containing raw 16-bit PCM audio samples
     * @param bytesRead number of valid bytes in {@code data}
     */
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
                    // Update the stats
                    totalBytesSent += bytesRead;
                    bytesInCurrentSecond += bytesRead;
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
