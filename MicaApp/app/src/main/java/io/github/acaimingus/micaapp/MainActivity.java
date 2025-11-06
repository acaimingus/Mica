package io.github.acaimingus.micaapp;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
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
import com.google.android.material.slider.Slider;
import com.google.android.material.slider.Slider.OnChangeListener;
import com.google.android.material.switchmaterial.SwitchMaterial;
import com.google.android.material.textview.MaterialTextView;

public class MainActivity extends AppCompatActivity {

    /**
     * Boolean specifying if the microphone is muted or not
     */
    private boolean isMuted = false;
    /**
     * Boolean specifying if the microphone is connected or not
     */
    private boolean isConnected = false;
    /**
     * Integer specifying the gain of the microphone
     */
    private int gain = 100;

    /**
     * Material button for the mute toggle
     */
    MaterialButton muteButton;
    /**
     * Material switch for the connection toggle
     */
    SwitchMaterial connectionSwitch;
    /**
     * Material text view for the connection status
     */
    MaterialTextView connectionStatus;
    /**
     * Material slider for the gain
     */
    Slider gainSlider;
    /**
     * Material text view for the gain
     */
    MaterialTextView gainText;

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

        // Get the views from the layout
        muteButton = findViewById(R.id.muteButton);
        connectionSwitch = findViewById(R.id.connectionSwitch);
        connectionStatus = findViewById(R.id.connectionStatus);
        gainSlider = findViewById(R.id.gainSlider);
        gainText = findViewById(R.id.gainText);

        // Prefill the text for the gain
        gainText.setText(R.string.hundred_percent);

        if (ContextCompat.checkSelfPermission(this, android.Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            connectionSwitch.setEnabled(false);
            ActivityCompat.requestPermissions(this, new String[]{android.Manifest.permission.RECORD_AUDIO}, 101);
        } else {
            connectionSwitch.setEnabled(true);
        }

        // Add a listener to the connection switch
        connectionSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                Intent serviceLaunchIntent = new Intent(this, MicrophoneService.class);
                startForegroundService(serviceLaunchIntent);
                isConnected = true;
                connectionStatus.setText(R.string.connected);
            } else {
                isConnected = false;
                Intent serviceStopIntent = new Intent(this, MicrophoneService.class);
                stopService(serviceStopIntent);
                connectionStatus.setText(R.string.not_connected);
            }
        });

        // Add a listener to the gain slider
        OnChangeListener sliderListener = (slider, value, fromUser) -> {
            gainText.setText(String.format("%s %%", (int) value));
            gain = (int) value;
        };
        gainSlider.addOnChangeListener(sliderListener);
    }

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
     * Method for toggling the mute button
     */
    public void muteToggle(View view) {
        if (!isMuted) {
            isMuted = true;
            muteButton.setText(R.string.unmute);
            muteButton.setIconResource(R.drawable.mic_24px);
        } else {
            isMuted = false;
            muteButton.setText(R.string.mute);
            muteButton.setIconResource(R.drawable.mic_off_24px);
        }
    }
}