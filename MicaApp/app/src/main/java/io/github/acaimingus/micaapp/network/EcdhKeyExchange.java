package io.github.acaimingus.micaapp.network;

import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.PublicKey;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.X509EncodedKeySpec;
import javax.crypto.KeyAgreement;
import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;
import android.util.Log;

public class EcdhKeyExchange {
    private static final String TAG = "EcdhKeyExchange";
    private KeyPair keyPair;

    public EcdhKeyExchange() {
        try {
            KeyPairGenerator kpg = KeyPairGenerator.getInstance("X25519");
            this.keyPair = kpg.generateKeyPair();
        } catch (NoSuchAlgorithmException e) {
            Log.e(TAG, "X25519 not supported on this device", e);
        }
    }

    /**
     * Gets the raw 32 bytes of the X25519 public key.
     * Note: X509EncodedKeySpec usually adds an ASN.1 header (12 bytes for X25519).
     * We strip it to send just the 32-byte raw key.
     */
    public byte[] getPublicKey() {
        if (keyPair == null) return new byte[32];
        byte[] encoded = keyPair.getPublic().getEncoded();
        // The raw key is the last 32 bytes of the encoded X.509 format
        byte[] rawKey = new byte[32];
        System.arraycopy(encoded, encoded.length - 32, rawKey, 0, 32);
        return rawKey;
    }

    /**
     * Computes the shared secret using the peer's raw 32-byte public key.
     */
    public byte[] computeSharedSecret(byte[] peerRawKey) {
        if (keyPair == null || peerRawKey.length != 32) return null;
        try {
            // Reconstruct the X.509 SubjectPublicKeyInfo header for X25519
            byte[] x509Header = new byte[]{
                    0x30, 0x2A, 0x30, 0x05, 0x06, 0x03, 0x2B, 0x65, 0x6E, 0x03, 0x21, 0x00
            };
            byte[] encodedKey = new byte[x509Header.length + peerRawKey.length];
            System.arraycopy(x509Header, 0, encodedKey, 0, x509Header.length);
            System.arraycopy(peerRawKey, 0, encodedKey, x509Header.length, peerRawKey.length);

            KeyFactory kf = KeyFactory.getInstance("X25519");
            X509EncodedKeySpec pkSpec = new X509EncodedKeySpec(encodedKey);
            PublicKey peerPublicKey = kf.generatePublic(pkSpec);

            KeyAgreement keyAgreement = KeyAgreement.getInstance("X25519");
            keyAgreement.init(keyPair.getPrivate());
            keyAgreement.doPhase(peerPublicKey, true);
            
            return keyAgreement.generateSecret();
        } catch (NoSuchAlgorithmException | InvalidKeySpecException | InvalidKeyException e) {
            Log.e(TAG, "Failed to compute shared secret", e);
            return null;
        }
    }

    /**
     * Derives a 6-digit PIN from the shared secret using SHA-256.
     */
    public static String derivePinFromSecret(byte[] secret) {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] hash = digest.digest(secret);

            long val = ((hash[0] & 0xFFL) << 24) |
                       ((hash[1] & 0xFFL) << 16) |
                       ((hash[2] & 0xFFL) << 8) |
                       (hash[3] & 0xFFL);
            long pin = val % 1000000;

            String pinStr = String.valueOf(pin);
            while (pinStr.length() < 6) {
                pinStr = "0" + pinStr;
            }
            return pinStr;
        } catch (NoSuchAlgorithmException e) {
            Log.e(TAG, "SHA-256 not supported", e);
            return "000000";
        }
    }

    /**
     * Generates an authentication token for the stream using HMAC-SHA256.
     */
    public static byte[] generateAuthToken(byte[] secret) {
        if (secret == null) return new byte[32];
        try {
            Mac mac = Mac.getInstance("HmacSHA256");
            SecretKeySpec keySpec = new SecretKeySpec(secret, "HmacSHA256");
            mac.init(keySpec);
            return mac.doFinal("MICA_STREAM".getBytes());
        } catch (NoSuchAlgorithmException | InvalidKeyException e) {
            Log.e(TAG, "HMAC-SHA256 failed", e);
            return new byte[32];
        }
    }
}
