package io.github.acaimingus.micaapp.network;

/**
 * Simple enum for representing the states a connection can be in
 */
public enum ConnectionStates {
    /**
     * Represents the connected state
     */
    CONNECTED,
    /**
     * Represents the transient state when connecting
     */
    CONNECTING,
    /**
     * Represents the disconnected state
     */
    DISCONNECTED
}
