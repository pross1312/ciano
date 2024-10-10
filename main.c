#include <stdio.h>
#include <math.h>
#include <raylib.h>

#define MAX_SAMPLES_PER_UPDATE   4096
#define SAMPLE_RATE 44100
#define NOTE_INC 1.0594630943592953f;
#define BASE_FREQUENCY 261.63f
#define MAX_AMPLITUDE 10000.0f

// Audio frequency, for smoothing
float audioFrequency = BASE_FREQUENCY;

#define NOTES_COUNT 12
bool notes[NOTES_COUNT]      = {0};
float sineIdxes[NOTES_COUNT] = {0};
const char note_keys[NOTES_COUNT] = {
    'Q', '2', 'W', '3', 'E', 'R', '5', 'T', '6', 'Y', '7', 'U'
};

// Previous value, used to test if sine needs to be rewritten, and to smoothly modulate frequency
float oldFrequency = 1.0f;

// Index for audio rendering
float sineIdx = 0.0f;

void AudioInputCallback(void *buffer, unsigned int frames)
{
    short *d = (short *)buffer;
    // audioFrequency = frequency + (audioFrequency - frequency)*0.95f;
    // float incr = audioFrequency*1.0f/SAMPLE_RATE;

    // for (unsigned int i = 0; i < frames; i++)
    // {
    //     d[2*i] = (short)(10000.0f*sinf(2*PI*sineIdx));
    //     d[2*i+1] = (short)(10000.0f*sinf(2*PI*sineIdx)); 
    //     sineIdx += incr;
    //     if (sineIdx > 1.0f) sineIdx -= 1.0f;
    // }
    for (unsigned int i = 0; i < frames; i++) {
        float amplitude = 0.0f;
        float audioFrequency = BASE_FREQUENCY;
        for (size_t i = 0; i < NOTES_COUNT; i++) {
            if (notes[i]) {
                const float incr = audioFrequency/SAMPLE_RATE;
                amplitude = MAX_AMPLITUDE*sinf(2*PI*sineIdxes[i]);
                sineIdxes[i] += incr;
                if (sineIdxes[i] > 1.0f) sineIdxes[i] -= 1.0f;
            }
            audioFrequency *= NOTE_INC;
        }
        d[2*i] = d[2*i+1] = amplitude;
    }
}

int main() {
    int factor = 100;
    InitWindow(16*factor, 9*factor, "Testing sound");
    InitAudioDevice();

    SetAudioStreamBufferSizeDefault(MAX_SAMPLES_PER_UPDATE);
    AudioStream stream = LoadAudioStream(SAMPLE_RATE, 16, 2);
    SetAudioStreamCallback(stream, AudioInputCallback);

    PlayAudioStream(stream);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        for (size_t i = 0; i < NOTES_COUNT; i++) {
            if (IsKeyDown(note_keys[i])) {
                notes[i] = true;
            } else {
                notes[i] = false;
                sineIdxes[i] = 0.0f;
            }
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
