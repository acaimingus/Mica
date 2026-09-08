package io.github.acaimingus.micaapp.network;

import android.content.Context;
import android.os.Build;
import android.provider.Settings;

public class DeviceIdentification {
    public static String getDeviceName(Context context)
    {
        var deviceModel = Build.MANUFACTURER + " " + Build.MODEL;

        // Try getting the name of the device name set by the user from the settings
        try {
            var deviceName = Settings.Secure.getString(context.getContentResolver(), "bluetooth_name");
            // If it was successful, then return the name set by the user + the model in brackets
            if (deviceName != null)
            {
                return deviceName + " (" + deviceModel + ")";
            }
        } catch (Exception exception)
        {
            // Failed to get the device name from the settings
        }

        // If it was not possible to get the device name through the settings then just get the model name
        return deviceModel;
    }
}
