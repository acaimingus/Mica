package io.github.acaimingus.micaapp.service;

import android.os.Binder;

public class LocalBinder extends Binder {
    private final MicrophoneService service;

    public LocalBinder(MicrophoneService service) {
        this.service = service;
    }

    public MicrophoneService getService() {
        return service;
    }
}
