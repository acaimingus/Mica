package io.github.acaimingus.micaapp.network;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Base64;

public class KeystoreManager {
    private static final String PREF_NAME = "MicaSecurityPrefs";
    private static final String KEY_SHARED_SECRET = "shared_secret";

    public static void saveSharedSecret(Context context, byte[] secret) {
        SharedPreferences prefs = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        String encoded = Base64.encodeToString(secret, Base64.DEFAULT);
        prefs.edit().putString(KEY_SHARED_SECRET, encoded).apply();
    }

    public static byte[] loadSharedSecret(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        String encoded = prefs.getString(KEY_SHARED_SECRET, null);
        if (encoded != null) {
            return Base64.decode(encoded, Base64.DEFAULT);
        }
        return null;
    }
}
