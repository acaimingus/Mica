package io.github.acaimingus.micaapp.service.audio;

public class AudioProcessor {
    public int gain = 100;
    public int gainFixed = 4096;
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
