package io.github.acaimingus.micaapp.audio;

/**
 * Processor class for applying gain/volume adjustments to raw 16-bit PCM audio data.
 * Uses fixed-point arithmetic to avoid floating-point overhead during audio processing.
 */
public class AudioProcessor {
    /**
     * Gain percentage to apply to the audio (100 = no change, 200 = double the volume)
     */
    public int gain = 100;

    /**
     * Fixed-point representation of the gain factor, scaled by 4096 (i.e. 2^12).
     * Calculated as {@code (gain / 100.0) * 4096}.
     * A shift of 12 bits is used after multiplication to convert back to the correct range.
     */
    public int gainFixed = 4096;

    /**
     * Applies the current gain to the provided raw 16-bit PCM audio buffer in-place.
     * If {@link #gain} is 100 (no change), the method returns immediately without modifying the data.
     * The result is clamped to the valid 16-bit signed range [-32768, 32767] to prevent clipping.
     *
     * @param data      byte array containing 16-bit little-endian PCM audio samples
     * @param bytesRead number of valid bytes in {@code data} to process
     */
    public void applyGain(byte[] data, int bytesRead) {
        // No gain to apply
        if (gain == 100) {
            return;
        }

        // 2 steps because we are working with 16 bit
        for (int i = 0; i < bytesRead; i+= 2) {
            // Combine 2 bytes to a 16 bit integer
            int audioSample = (data[i] & 0xFF) | (data[i+1] << 8);
            // Apply gain, apply our factor and divide by 4096
            int newSample = (audioSample * gainFixed) >> 12;
            // Prevent clipping by bounding the values
            if (newSample > 32767) {
                newSample = 32767;
            } else if (newSample < -32768) {
                newSample = -32768;
            }
            // Put the result back in the array
            data[i] = (byte) newSample;
            data[i+1] = (byte) (newSample >> 8);
        }
    }
}
