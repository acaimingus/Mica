package io.github.acaimingus.micaapp.activity;

import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.view.View;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.divider.MaterialDivider;
import com.google.android.material.slider.Slider;
import com.google.android.material.slider.Slider.OnChangeListener;
import com.google.android.material.switchmaterial.SwitchMaterial;
import com.google.android.material.textview.MaterialTextView;

import java.util.ArrayList;
import java.util.List;

import io.github.acaimingus.micaapp.R;
import io.github.acaimingus.micaapp.network.ConnectionCallbacks;
import io.github.acaimingus.micaapp.network.ConnectionStates;
import io.github.acaimingus.micaapp.network.DeviceIdentification;
import io.github.acaimingus.micaapp.service.LocalBinder;
import io.github.acaimingus.micaapp.service.MicrophoneService;

/**
 * Main activity for the app
 */
public class MainActivity extends AppCompatActivity implements ConnectionCallbacks {
    /**
     * Material button for the mute toggle
     */
    private MaterialButton muteButton;

    /**
     * Material switch for the connection toggle
     */
    private SwitchMaterial connectionSwitch;

    /**
     * Material text view for the connection status
     */
    private MaterialTextView connectionStatus;

    /**
     * Material text view for the gain
     */
    private MaterialTextView gainText;
    /**
     * Material text view for the stats
     */
    private MaterialTextView statsText;

    /**
     * Material text view for displaying the device name
     */
    private MaterialTextView deviceNameText;

    /**
     * Material divider for the stats, gets toggled when stats are visible
     */
    private MaterialDivider statsDivider;
    /**
     * Reference to the microphone service
     */
    private MicrophoneService microphoneService;
    /**
     * Boolean specifying whether the service is bound or not
     */
    private boolean isServiceBound = false;

    /**
     * Reference to this activity for the runnables in the callbacks
     */
    MainActivity mainActivity = this;

    /**
     * Intent for the service
     */
    Intent serviceIntent;

    /**
     * State of the connection as known by the UI;
     * ONLY SET THIS IN THE CONNECTION CALLBACKS TO PRESERVE THE SINGLE SOURCE OF TRUTH!
     */
    ConnectionStates currentConnectionState = ConnectionStates.DISCONNECTED;

    /**
     * AlertDialog for showing the pairing code when a connection gets initialized
     */
    private androidx.appcompat.app.AlertDialog pairingDialog;

    /**
     * Service connection that binds {@link MainActivity} to {@link MicrophoneService}.
     * On connection, registers this activity as the connection callback and syncs the UI.
     * On disconnection, clears the callback and releases the service reference.
     */
    private final ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            LocalBinder binder = (LocalBinder) service;
            microphoneService = binder.getService();
            isServiceBound = true;
            // Set a callback for updating the UI when the connection state changes
            microphoneService.setConnectionCallbacks(MainActivity.this);
            updateUiState();
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            // Unset the callback on disconnect
            if (microphoneService != null) {
                microphoneService.setConnectionCallbacks(null);
            }
            microphoneService = null;
            isServiceBound = false;
        }
    };

    /**
     * Method for constructing all necessary components when the activity is started.
     *
     * @param savedInstanceState If the activity is being re-initialized after
     *                           previously being shut down then this Bundle contains the data it most
     *                           recently supplied in {@link #onSaveInstanceState}.  <b><i>Note: Otherwise it is null.</i></b>
     *
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        // Create the service intent
        serviceIntent = new Intent(this, MicrophoneService.class);

        // Get the views from the layout
        muteButton = findViewById(R.id.muteButton);
        connectionSwitch = findViewById(R.id.connectionSwitch);
        connectionStatus = findViewById(R.id.connectionStatus);
        Slider gainSlider = findViewById(R.id.gainSlider);
        gainText = findViewById(R.id.gainText);
        statsText = findViewById(R.id.statsText);
        statsDivider = findViewById(R.id.statsDivider);
        deviceNameText = findViewById(R.id.deviceNameText);


        // Prefill the text for the gain
        gainText.setText(R.string.hundred_percent);

        // Get the needed permissions for Android 13 and up
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // List of needed permissions
            List<String> permissionsToRequest = new ArrayList<>();

            // Microphone permissions
            if (ContextCompat.checkSelfPermission(this, android.Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
                permissionsToRequest.add(android.Manifest.permission.RECORD_AUDIO);
            }

            // Notification permissions
            if (ContextCompat.checkSelfPermission(this, android.Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
                permissionsToRequest.add(android.Manifest.permission.POST_NOTIFICATIONS);
            }

            if (!permissionsToRequest.isEmpty()) {
                connectionSwitch.setEnabled(false);
                ActivityCompat.requestPermissions(this, permissionsToRequest.toArray(new String[0]), 101);
            } else {
                connectionSwitch.setEnabled(true);
            }
        } else {
            // Android 12 and below
            if (ContextCompat.checkSelfPermission(this, android.Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
                connectionSwitch.setEnabled(false);
                ActivityCompat.requestPermissions(this, new String[]{android.Manifest.permission.RECORD_AUDIO}, 101);
            } else {
                connectionSwitch.setEnabled(true);
            }
        }

        // Add a listener to the connection switch
        connectionSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked && currentConnectionState == ConnectionStates.DISCONNECTED) {
                onConnecting();
            } else if (!isChecked && currentConnectionState != ConnectionStates.DISCONNECTED) {
                onDisconnected();
            }
        });

        // Add a listener to the gain slider
        OnChangeListener sliderListener = (slider, value, fromUser) -> {
            int valueInt = (int) value;
            gainText.setText(String.format("%s %%", valueInt));
            microphoneService.setGain(valueInt);
        };
        gainSlider.addOnChangeListener(sliderListener);
    }

    /**
     * Called when the activity is being destroyed. Unbinds from the microphone service
     * to prevent resource leaks.
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (isServiceBound) {
            unbindService(serviceConnection);
            isServiceBound = false;
        }
    }

    /**
     * Method for handling the result of the permission request.
     *
     * @param requestCode  The request code passed in {@link #requestPermissions}.
     * @param permissions  The requested permissions. Never null.
     * @param grantResults The grant results for the corresponding permissions which is either
     *                     {@link android.content.pm.PackageManager#PERMISSION_GRANTED} or
     *                     {@link android.content.pm.PackageManager#PERMISSION_DENIED}. Never null.
     *
     */
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == 101) {
            // Check if the permission was given
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                // Permission was granted
                connectionSwitch.setEnabled(true);
            } else {
                // permission not given
                Toast.makeText(this, R.string.microphone_permission_denied, Toast.LENGTH_SHORT).show();
                connectionSwitch.setEnabled(false);
            }
        }
    }

    /**
     * Synchronises the UI state (mute button and connection switch) with the current state
     * of the bound {@link MicrophoneService}. Has no effect if the service is not yet bound.
     */
    private void updateUiState() {
        if (!isServiceBound || microphoneService == null) {
            return;
        }
        updateMuteButtonUI(microphoneService.getIsMuted());
        connectionSwitch.setChecked(MicrophoneService.isRunning);
    }

    /**
     * Updates the mute button's label and icon to reflect whether the microphone is muted.
     *
     * @param isMuted {@code true} if the microphone is currently muted
     */
    private void updateMuteButtonUI(boolean isMuted) {
        if (isMuted) {
            muteButton.setText(R.string.unmute);
            muteButton.setIconResource(R.drawable.mic_24px);
        } else {
            muteButton.setText(R.string.mute);
            muteButton.setIconResource(R.drawable.mic_off_24px);
        }
    }

    /**
     * Method for toggling the mute button
     *
     * @param view The view that was clicked
     */
    public void muteToggle(View view) {
        if (isServiceBound && microphoneService != null) {
            boolean newState = !microphoneService.getIsMuted();
            microphoneService.setIsMuted(newState);
            updateMuteButtonUI(newState);
        }
    }

    /**
     * Called when a desktop listener has successfully connected.
     * Updates the connection status text view and shows a toast notification.
     */
    @Override
    public void onConnected() {
        if (currentConnectionState == ConnectionStates.CONNECTED) {
            return;
        }
        currentConnectionState = ConnectionStates.CONNECTED;
        runOnUiThread(() -> {
            connectionStatus.setText(R.string.connected);
            // Show a little toast to the user
            Toast.makeText(mainActivity, "Connection established!", Toast.LENGTH_SHORT).show();
        });
    }

    /**
     * Called when the service is attempting to establish a connection.
     * Updates the connection status text view, starts and binds to the microphone service.
     */
    @Override
    public void onConnecting() {
        if (currentConnectionState == ConnectionStates.CONNECTING) {
            return;
        }
        currentConnectionState = ConnectionStates.CONNECTING;
        runOnUiThread(() -> {
            connectionStatus.setText(R.string.connecting);
            ContextCompat.startForegroundService(mainActivity, serviceIntent);
            bindService(serviceIntent, serviceConnection, Context.BIND_AUTO_CREATE);
        });
    }

    /**
     * Called when the connection to the desktop listener has been lost or closed.
     * Updates the connection status text view, shows a toast notification, unbinds from
     * the service and resets the statistics display.
     */
    @Override
    public void onDisconnected() {
        // Exit
        if (currentConnectionState == ConnectionStates.DISCONNECTED) {
            return;
        }
        currentConnectionState = ConnectionStates.DISCONNECTED;
        runOnUiThread(() -> {
            connectionStatus.setText(R.string.not_connected);
            connectionSwitch.setChecked(false);
            // Show a little toast to the user
            Toast.makeText(mainActivity, "Connection lost or closed.", Toast.LENGTH_SHORT).show();
            if (isServiceBound) {
                if (microphoneService != null) {
                    microphoneService.setConnectionCallbacks(null);
                }
                unbindService(serviceConnection);
                isServiceBound = false;
            }
            // Reset the stats text and hide it
            statsText.setText("");
            toggleStatsVisibility(false);
            stopService(serviceIntent);
        });
    }

    private void toggleStatsVisibility(boolean toggle) {
        if (toggle) {
            statsText.setVisibility(VISIBLE);
            deviceNameText.setVisibility(VISIBLE);
            statsDivider.setVisibility(VISIBLE);
        } else {
            statsText.setVisibility(INVISIBLE);
            deviceNameText.setVisibility(INVISIBLE);
            statsDivider.setVisibility(INVISIBLE);
        }
    }

    /**
     * Called periodically (approximately once per second) with updated network statistics.
     * Formats and displays the network statistics in the stats text view and makes the
     * stats divider visible.
     *
     * @param totalBytesSent        cumulative number of bytes sent since the service started
     * @param currentBytesPerSecond number of bytes sent in the most recent second
     */
    @Override
    public void onNetworkStatsUpdated(long totalBytesSent, int currentBytesPerSecond) {
        double kbPerSecond = currentBytesPerSecond / 1024.0;
        double totalMb = totalBytesSent / (1024.0 * 1024.0);

        String stats = String.format(getString(R.string.rate_1f_kb_s_total_1f_mb), kbPerSecond, totalMb);

        statsText.setText(stats);
        deviceNameText.setText(DeviceIdentification.getDeviceName(this));
        toggleStatsVisibility(true);
    }

    @Override
    public void onPairingRequested(String pin) {
        runOnUiThread(() -> {
            if (pairingDialog != null && pairingDialog.isShowing()) {
                pairingDialog.dismiss();
            }
            pairingDialog = new androidx.appcompat.app.AlertDialog.Builder(this)
                    .setTitle("Pairing Request")
                    .setMessage("A device wants to pair.\nPlease confirm that the following code is displayed on the device:\n\n" + pin)
                    .setPositiveButton("Allow", (dialog, which) -> {
                        if (isServiceBound && microphoneService != null) {
                            microphoneService.confirmPairing();
                        }
                    })
                    .setNegativeButton("Deny", (dialog, which) -> {
                        if (isServiceBound && microphoneService != null) {
                            microphoneService.rejectPairing();
                        }
                    })
                    .setCancelable(false)
                    .create();
            pairingDialog.show();
        });
    }

    @Override
    public void onPairingRejected() {
        runOnUiThread(() -> {
            if (pairingDialog != null && pairingDialog.isShowing()) {
                pairingDialog.dismiss();
                pairingDialog = null;
                // Show a toast that pairing was rejected
                android.widget.Toast.makeText(this, "Pairing cancelled by the other device.", android.widget.Toast.LENGTH_SHORT).show();
            }
        });
    }
}