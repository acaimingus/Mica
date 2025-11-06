package io.github.acaimingus.micaapp;

import android.Manifest;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;
import androidx.annotation.RequiresPermission;

public class MicrophoneController {

    private AudioRecord audioRecord;
    private Thread recorderThread;
    private boolean isRecording;

    private final int sampleRate = 44100;
    private final int channelConfiguration = AudioFormat.CHANNEL_IN_MONO;
    private final int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
    private final int minimumBufferSize = AudioRecord.getMinBufferSize(sampleRate, channelConfiguration, audioFormat);;

    @RequiresPermission(Manifest.permission.RECORD_AUDIO)
    public void startRecording() {
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, sampleRate, channelConfiguration, audioFormat, minimumBufferSize * 2);
        isRecording = true;
        audioRecord.startRecording();

        // Start a thread for reading the recorder
        recorderThread = new Thread(() -> {
            byte[] buffer = new byte[minimumBufferSize];

            while (isRecording) {
                int bytesRead = audioRecord.read(buffer, 0, minimumBufferSize);
                if (bytesRead > 0) {
                    long sum = 0;
                    for (int i = 0; i < bytesRead; i++) {
                        sum += Math.abs(buffer[i]);
                    }
                    double average = (double) sum / bytesRead;
                    Log.d("MicrophoneController", "Audio data read: " + average);
                }
            }
        });
        recorderThread.start();
    }

    public void stopRecording() {
        // Set the recording status to false
        isRecording = false;

        // Stop the thread
        try {
            // Wait for the recorder thread to exit
            if (recorderThread != null) {
                recorderThread.join();
            }
        } catch (InterruptedException exception) {
            Log.e("MicrophoneController", "Error stopping recorder thread", exception);
        }

        // Free the AudioRecord instance
        if (audioRecord != null) {
            audioRecord.stop();
            audioRecord.release();
            audioRecord = null;
        }
    }
}
