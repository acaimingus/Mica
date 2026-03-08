package io.github.acaimingus.micaapp.activity;

/**
 * Callback interface for receiving connection state changes from {@link io.github.acaimingus.micaapp.service.MicrophoneService}.
 * Implemented by the UI layer (e.g. {@link MainActivity}) to update the interface in response
 * to network events.
 */
public interface ConnectionCallbacks {
    /**
     * Called when a desktop listener has successfully connected.
     */
    void onConnected();

    /**
     * Called when the service is attempting to establish a connection.
     */
    void onConnecting();

    /**
     * Called when the connection to the desktop listener has been lost or closed.
     */
    void onDisconnected();

    /**
     * Called periodically (approximately once per second) with updated network statistics.
     *
     * @param totalBytesSent        cumulative number of bytes sent since the service started
     * @param currentBytesPerSecond number of bytes sent in the most recent second
     */
    void onNetworkStatsUpdated(long totalBytesSent, int currentBytesPerSecond);
}
