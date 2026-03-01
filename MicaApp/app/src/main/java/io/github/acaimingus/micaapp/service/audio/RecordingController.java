package io.github.acaimingus.micaapp.service.audio;

import android.Manifest;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;
import androidx.annotation.RequiresPermission;

/**
 * Controller class for managing the microphone recording.
 */
public class RecordingController {

    /**
     * AudioRecord-instance for recording the microphone on Android
     */
    private AudioRecord audioRecord;
    /**
     * Thread for reading the audio recorder output
     */
    private Thread recorderThread;
    /**
     * Flag to indicate if the microphone is currently recording
     */
    private boolean isRecording;
    /**
     * Sample rate for the microphone recording
     */
    private final int sampleRate = 44100;
    /**
     * Channel configuration for the microphone recording
     */
    private final int channelConfiguration = AudioFormat.CHANNEL_IN_MONO;
    /**
     * Audio format for the microphone recording
     */
    private final int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
    /**
     * Minimum buffer size for the microphone recording
     */
    private final int minimumBufferSize = AudioRecord.getMinBufferSize(sampleRate, channelConfiguration, audioFormat);
    /**
     * Interface for passing the recorded data along to the Service for transmission
     */
    private IAudioDataListener audioDataListener;

    /**
     * Setter the listener to pass the recorded data to
     * @param listener Listener to pass the recorded data to
     */
    public void setAudioDataListener(IAudioDataListener listener) {
        this.audioDataListener = listener;
    }

    /**
     * Method for handling the start of recording on the phone
     */
    @RequiresPermission(Manifest.permission.RECORD_AUDIO)
    public void startRecording() {
        // Create a new AudioRecord instance with the specified parameters
        // Use twice the minimum buffer size just to be safe
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, sampleRate, channelConfiguration, audioFormat, minimumBufferSize * 2);
        // Set the recording flag
        isRecording = true;
        // Start recording
        audioRecord.startRecording();

        // Create a thread for reading the recorded audio data
        recorderThread = new Thread(() -> {
            // Create a buffer for reading the audio data
            byte[] buffer = new byte[minimumBufferSize];
            while (isRecording) {
                int bytesRead = audioRecord.read(buffer, 0, minimumBufferSize);
                if (bytesRead > 0) {
                    if (audioDataListener != null) {
                        // Pass the data over to the Service for transmission
                        audioDataListener.onAudioDataReceived(buffer, bytesRead);
                    }
                }
            }
        });
        // Start the thread
        recorderThread.start();
    }

    /**
     * Method for handling the stop of recording on the phone
     */
    public void stopRecording() {
        // Set the recording status to false
        isRecording = false;

        // Stop the thread
        try {
            // Wait for the recorder thread to exit
            if (recorderThread != null) {
                recorderThread.join();
                recorderThread = null;
            }
        } catch (InterruptedException exception) {
            Log.e("RecordingController", "Error stopping recorder thread", exception);
        }

        // Free the listener
        audioDataListener = null;

        // Free the AudioRecord instance
        if (audioRecord != null) {
            audioRecord.stop();
            audioRecord.release();
            audioRecord = null;
        }
    }
}
