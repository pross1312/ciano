#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <raylib.h>

#define MAX_SAMPLES_PER_UPDATE   4096
#define SAMPLE_RATE 44100
#define NOTE_INC 1.0594630943592953f // 2^12
#define BASE_FREQUENCY 261.63f
#define MAX_AMPLITUDE 5000.0f
#define FADE_TIME 0.15f // second
#define FADE_SAMPLE (SAMPLE_RATE*FADE_TIME)
#define AMP_PER_SAMPLE (MAX_AMPLITUDE/FADE_SAMPLE)

typedef enum {
    PHASE_STOP = 0,
    PHASE_FADE_IN,
    PHASE_PLAYING,
    PHASE_FADE_OUT,
} Phase;

typedef struct {
    float frequency;
    float sineidx;
    float amplitude;
    uint32_t phase_sample_played;
    Phase phase;
} Note;

#define NOTES_COUNT 12
Note notes[NOTES_COUNT] = {0};
const char note_keys[NOTES_COUNT] = {
    'Q', '2', 'W', '3', 'E', 'R', '5', 'T', '6', 'Y', '7', 'U'
};

void start_note(Note *note) {
    note->phase = PHASE_FADE_IN;
    note->phase_sample_played = 0;
    note->amplitude = 0;
    note->sineidx = 0;
}
void stop_note(Note *note) {
    if (note->phase == PHASE_PLAYING) {
        note->phase = PHASE_FADE_OUT;
        note->phase_sample_played = 0;
    } else if (note->phase == PHASE_FADE_IN) {
        note->phase = PHASE_FADE_OUT;
        note->phase_sample_played = FADE_SAMPLE - note->phase_sample_played;
    } else {
        assert(false && "Invalid pre phase into fade out");
    }
}

// 44100 sample/second
float get_note_amplitude(Note *note) {

    float result = 0;
    switch (note->phase) {
    case PHASE_STOP:
        return 0;
        break;
    case PHASE_FADE_IN:
        if (note->phase_sample_played >= FADE_SAMPLE) {
            note->phase = PHASE_PLAYING;
            note->phase_sample_played = 0;
            note->amplitude += AMP_PER_SAMPLE;
        } else {
            note->amplitude = note->phase_sample_played/FADE_SAMPLE * MAX_AMPLITUDE;
        }
        break;
    case PHASE_PLAYING:
        note->amplitude = MAX_AMPLITUDE;
        break;
    case PHASE_FADE_OUT:
        if (note->phase_sample_played >= FADE_SAMPLE) {
            note->phase = PHASE_STOP;
            note->phase_sample_played = 0;
            note->phase_sample_played = 0;
        } else {
            note->amplitude -= AMP_PER_SAMPLE;
        }
        break;
    }

    result = note->amplitude * sinf(2*PI*note->sineidx);
    note->sineidx += note->frequency/SAMPLE_RATE;
    if (note->sineidx > 1.0f) note->sineidx -= 1.0f;
    note->phase_sample_played += 1;
    return result;
}

void AudioInputCallback(void *buffer, unsigned int samples)
{
    int16_t *d = (int16_t *)buffer;
    for (unsigned int i = 0; i < samples; i++) {
        int16_t amplitude = 0.0f;
        for (size_t i = 0; i < NOTES_COUNT; i++) {
            amplitude += (int16_t)get_note_amplitude(&notes[i]);
        }
        d[2*i] = d[2*i+1] = amplitude;
    }
}

int main() {
    int factor = 100;
    InitWindow(16*factor, 9*factor, "Testing sound");
    InitAudioDevice();

    float fre = BASE_FREQUENCY;
    for (size_t i = 0; i < NOTES_COUNT; i++) {
        notes[i].frequency = fre;
        fre *= NOTE_INC;
    }

    SetAudioStreamBufferSizeDefault(MAX_SAMPLES_PER_UPDATE);
    AudioStream stream = LoadAudioStream(SAMPLE_RATE, 16, 2);
    SetAudioStreamCallback(stream, AudioInputCallback);

    PlayAudioStream(stream);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        for (size_t i = 0; i < NOTES_COUNT; i++) {
            if (IsKeyPressed(note_keys[i])) {
                start_note(&notes[i]);
            } else if (IsKeyReleased(note_keys[i])) {
                stop_note(&notes[i]);
            }
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
