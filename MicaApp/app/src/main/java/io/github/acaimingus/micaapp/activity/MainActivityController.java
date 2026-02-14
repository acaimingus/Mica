package io.github.acaimingus.micaapp.activity;

import android.content.Intent;

import io.github.acaimingus.micaapp.service.MicrophoneService;

public class MainActivityController {

    /**
     * The associated view with this controller
     */
    private final MainActivity view;

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

    public MainActivityController(MainActivity mainActivity) {
        view = mainActivity;
    }

    /**
     * Setter for isMuted
     * @param muted Boolean value to set
     */
    public void setMuted(boolean muted) {
        isMuted = muted;
    }

    /**
     * Getter for isMuted
     * @return Value of isMuted
     */
    public boolean getMuted() {
        return isMuted;
    }

    /**
     * Setter for isConnected
     * @param connected Boolean value to set
     */
    public void setConnected(boolean connected) {
        isConnected = connected;
    }

    /**
     * Setter for the microphone gain
     * @param value Value to set the gain to
     */
    public void setGain(int value) {
        gain = value;
    }

    public void startMicrophoneService() {
        Intent serviceLaunchIntent = new Intent(view, MicrophoneService.class);
        view.startForegroundService(serviceLaunchIntent);
    }

    public void stopMicrophoneService() {
        Intent serviceStopIntent = new Intent(view, MicrophoneService.class);
        view.stopService(serviceStopIntent);
    }
}
