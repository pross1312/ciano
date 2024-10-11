#include <stdint.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <raylib.h>

#define MAX_SAMPLES_PER_UPDATE   412
#define SAMPLE_RATE 44100
#define NOTE_INC 1.0594630943592953f // 2^12
#define BASE_FREQUENCY 261.63f
#define MAX_AMPLITUDE 8000.0f
#define FADE_IN_TIME 0.15f // second
#define FADE_OUT_TIME 0.6f // second
#define FADE_IN_SAMPLE_COUNT (SAMPLE_RATE*FADE_IN_TIME)
#define FADE_OUT_SAMPLE_COUNT (SAMPLE_RATE*FADE_OUT_TIME)

typedef enum {
    PHASE_STOP = 0,
    PHASE_FADE_IN,
    PHASE_FADE_OUT,
    PHASE_FADE_DOWN,
    PHASE_FADE_UP,
    PHASE_PLAYING,
} Phase;

typedef struct {
    float frequency;
    float sineidx;
    float amplitude;
    float max_amplitude;
    Phase phase;
} Note;

size_t active_notes = 0;
const char note_keys[] = {
    'q', '2', 'w', '3', 'e', 'r', '5', 't', '6', 'y', '7', 'u',
    'a',
};
#define NOTES_COUNT sizeof(note_keys)/sizeof(*note_keys)
Note notes[NOTES_COUNT] = {0};


void start_note(Note *note) {
    active_notes += 1;
    note->phase = PHASE_FADE_IN;
}
void stop_note(Note *note) {
    active_notes -= 1;
    note->phase = PHASE_FADE_OUT;
}
void set_note_max_amplitude(Note *note, float new_max_amplitude) {
    if (note->amplitude > new_max_amplitude) {
        note->phase = PHASE_FADE_DOWN;
    } else if (note->amplitude < new_max_amplitude) {
        note->phase = PHASE_FADE_UP;
    }
    note->max_amplitude = new_max_amplitude;
}

// 44100 sample/second
float get_note_amplitude(Note *note) {
    // TraceLog(LOG_INFO, "%zu %f", active_notes, note->max_amplitude);
    float result = 0;
    switch (note->phase) {
    case PHASE_STOP:
        return 0;
        break;
    case PHASE_FADE_IN:
        if (note->amplitude >= note->max_amplitude) {
            note->phase = PHASE_PLAYING;
            note->amplitude = note->max_amplitude;
        } else {
            note->amplitude += note->max_amplitude/FADE_IN_SAMPLE_COUNT;
        }
        break;
    case PHASE_FADE_OUT:
        if (note->amplitude <= 0) {
            note->phase = PHASE_STOP;
            note->amplitude = 0;
            note->sineidx = 0;
        } else {
            note->amplitude -= note->max_amplitude/FADE_OUT_SAMPLE_COUNT;
        }
        break;
    case PHASE_FADE_UP:
        if (note->amplitude >= note->max_amplitude) {
            note->phase = PHASE_PLAYING;
            note->amplitude = note->max_amplitude;
        } else {
            if (active_notes == 1) {
                note->amplitude += note->max_amplitude/FADE_IN_SAMPLE_COUNT;
            } else {
                note->amplitude += (note->max_amplitude/FADE_IN_SAMPLE_COUNT)/(active_notes-1);
            }
        }
        break;
    case PHASE_FADE_DOWN:
        if (note->amplitude <= note->max_amplitude) {
            note->phase = PHASE_PLAYING;
            note->amplitude = note->max_amplitude;
        } else {
            if (active_notes == 1) {
                note->amplitude -= note->max_amplitude/FADE_IN_SAMPLE_COUNT;
            } else {
                note->amplitude -= (note->max_amplitude/FADE_IN_SAMPLE_COUNT)/(active_notes-1);
            }
        }
        break;
    case PHASE_PLAYING:
        note->amplitude = note->max_amplitude;
        break;
    }

    result = note->amplitude * (2.0f * fabsf(2.0f * note->sineidx - 1.0f) - 1.0f); // triangle wave
    // result = note->amplitude * (note->sineidx < 0.5f ? 1.0f : -1.0f); // square wave
    // result = note->amplitude * sinf(2*PI*note->sineidx);           // sine wave
    note->sineidx += note->frequency/SAMPLE_RATE;
    if (note->sineidx >= 1.0f) note->sineidx -= 1.0f;
    return result;
}

void AudioInputCallback(void *buffer, unsigned int samples)
{
    int16_t *d = (int16_t *)buffer;
    for (unsigned int i = 0; i < samples; i++) {
        float amplitude = 0.0f;
        for (size_t i = 0; i < NOTES_COUNT; i++) {
            if (notes[i].phase != PHASE_STOP) {
                amplitude += get_note_amplitude(&notes[i]);
            }
        }
        d[i] = (int16_t)amplitude;
    }
}

bool check_note_pressed(char note_key) {
    if (isdigit(note_key)) {
        return IsKeyPressed(note_key);
    } else if (toupper(note_key) != note_key) {
        return IsKeyUp(KEY_LEFT_SHIFT) && IsKeyUp(KEY_RIGHT_SHIFT) && IsKeyPressed(toupper(note_key));
    } else {
        return (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && IsKeyPressed(toupper(note_key));
    }
}

bool check_note_released(char note_key) {
    return IsKeyReleased(toupper(note_key));
}

int main() {
    int factor = 100;
    InitWindow(16*factor, 9*factor, "Testing sound");
    SetTargetFPS(60);
    InitAudioDevice();

    float fre = BASE_FREQUENCY;
    for (size_t i = 0; i < NOTES_COUNT; i++) {
        notes[i].frequency = fre;
        fre *= NOTE_INC;
    }

    SetAudioStreamBufferSizeDefault(MAX_SAMPLES_PER_UPDATE);
    AudioStream stream = LoadAudioStream(SAMPLE_RATE, 16, 1);
    SetAudioStreamCallback(stream, AudioInputCallback);

    PlayAudioStream(stream);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        for (size_t i = 0; i < NOTES_COUNT; i++) {
            if (check_note_pressed(note_keys[i])) {
                start_note(&notes[i]);
                notes[i].max_amplitude = MAX_AMPLITUDE/fmax(active_notes, 1);
                for (size_t j = 0; j < NOTES_COUNT; j++) {
                    if (i != j && notes[j].phase != PHASE_STOP && notes[j].phase != PHASE_FADE_OUT) {
                        set_note_max_amplitude(&notes[j], MAX_AMPLITUDE/fmax(active_notes, 1));
                    }
                }
            } else if (notes[i].phase != PHASE_STOP && check_note_released(note_keys[i])) {
                stop_note(&notes[i]);
                notes[i].max_amplitude = MAX_AMPLITUDE/fmax(active_notes, 1);
                for (size_t j = 0; j < NOTES_COUNT; j++) {
                    if (i != j && notes[j].phase != PHASE_STOP && notes[j].phase != PHASE_FADE_OUT) {
                        set_note_max_amplitude(&notes[j], MAX_AMPLITUDE/fmax(active_notes, 1));
                    }
                }
            }
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
