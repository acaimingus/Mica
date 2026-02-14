package io.github.acaimingus.micaapp.activity;

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

import io.github.acaimingus.micaapp.R;

/**
 * Main activity for the app
 */
public class MainActivity extends AppCompatActivity {
    /**
     * Controller for this view
     */
    private MainActivityController controller;

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
     * Material slider for the gain
     */
    private Slider gainSlider;

    /**
     * Material text view for the gain
     */
    private MaterialTextView gainText;

    /**
     * Method for constructing all necessary components when the activity is started.
     *
     * @param savedInstanceState If the activity is being re-initialized after
     *     previously being shut down then this Bundle contains the data it most
     *     recently supplied in {@link #onSaveInstanceState}.  <b><i>Note: Otherwise it is null.</i></b>
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

        // Create the controller
        controller = new MainActivityController(this);

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
                controller.startMicrophoneService();
                controller.setConnected(true);
                connectionStatus.setText(R.string.connected);
            } else {
                controller.stopMicrophoneService();
                controller.setConnected(false);
                connectionStatus.setText(R.string.not_connected);
            }
        });

        // Add a listener to the gain slider
        OnChangeListener sliderListener = (slider, value, fromUser) -> {
            int valueInt = (int) value;
            gainText.setText(String.format("%s %%", valueInt));
            controller.setGain(valueInt);
        };
        gainSlider.addOnChangeListener(sliderListener);
    }

    /**
     * Method for handling the result of the permission request.
     *
     * @param requestCode The request code passed in {@link #requestPermissions}.
     * @param permissions The requested permissions. Never null.
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
     * Method for toggling the mute button
     *
     * @param view The view that was clicked
     */
    public void muteToggle(View view) {
        if (!controller.getMuted()) {
            controller.setMuted(true);
            muteButton.setText(R.string.unmute);
            muteButton.setIconResource(R.drawable.mic_24px);
        } else {
            controller.setMuted(false);
            muteButton.setText(R.string.mute);
            muteButton.setIconResource(R.drawable.mic_off_24px);
        }
    }
}