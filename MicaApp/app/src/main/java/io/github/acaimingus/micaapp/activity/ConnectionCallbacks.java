package io.github.acaimingus.micaapp.activity;

public interface ConnectionCallbacks {
    void onConnected();
    void onConnecting();
    void onDisconnected();
    void onNetworkStatsUpdated(long totalBytesSent, int currentBytesPerSecond);
}
