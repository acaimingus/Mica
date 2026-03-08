package io.github.acaimingus.micaapp.service;

import android.os.Binder;

/**
 * Binder subclass that provides a direct reference to {@link MicrophoneService} to clients
 * that bind to it. Because the service always runs in the same process as its clients, the
 * binder can safely return the service object itself.
 */
public class LocalBinder extends Binder {
    /**
     * The {@link MicrophoneService} instance this binder is associated with
     */
    private final MicrophoneService service;

    /**
     * Creates a new LocalBinder for the given service.
     *
     * @param service the {@link MicrophoneService} to expose through this binder
     */
    public LocalBinder(MicrophoneService service) {
        this.service = service;
    }

    /**
     * Returns the {@link MicrophoneService} instance associated with this binder.
     *
     * @return the bound {@link MicrophoneService}
     */
    public MicrophoneService getService() {
        return service;
    }
}
