package io.github.acaimingus.micaapp.service;

/**
 * Interface for receiving audio data from the microphone and passing it to the service.
 */
public interface IAudioDataListener {
    /**
     * Handler method for the data received from the microphone.
     *
     * @param data data of the received message
     * @param bytesRead length of the received message
     */
    void onAudioDataReceived(byte[] data, int bytesRead);
}
