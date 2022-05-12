#include <emu.h>
#include <math.h>

#define F_CPU_NTSC  1789773
#define F_CPU_PAL   1662607

/* SDL audio callback function */
static void audio_callback(void *udata, uint8_t *stream, int len);

static SDL_AudioSpec audio;
static uint32_t mixer_ptr = 0;

/*static const float PULSE_DUTY[4] = {
    0.125f, 0.25f, 0.5f, 0.75f
};

static inline float pulse_harmonic(float t, float f, float d, int n) {
    float result = d;
    for (int k = 1; k <= n; k++) {
        result += (2.0 / (k * M_PI)) * sin(k * M_PI * d) * cos(2 * M_PI * f * k * t);
    }
    return result;
}*/

bool init_audio(void) {
    /* Set the audio format */
    audio.freq = 262144; //44100;
    audio.format = AUDIO_F32;
    audio.channels = 2;    /* 1 = mono, 2 = stereo */
    audio.samples = 1024;  /* Good low-latency value for callback */
    audio.callback = audio_callback;
    audio.userdata = NULL;

    if (SDL_OpenAudio(&audio, NULL) != 0) {
        printf("Couldn't open audio: %s\n", SDL_GetError());
        return false;
    }

    SDL_PauseAudio(0);
    return true;
}

static void audio_callback(void *udata, uint8_t *stream, int len) {
    float *output = (float*)stream;
    const int nsamples = len / sizeof(float);
    const int cpu_freq = tv_sys == TV_SYS_PAL ? F_CPU_PAL : F_CPU_NTSC;
    const double out_per_sec = cpu_freq / 2.0;
    const double calls_per_sec = (double)(audio.freq * audio.channels) / nsamples;
    const int data_to_fetch = out_per_sec / calls_per_sec;
    for (int i = 0; i < nsamples; i++) {
        output[i] = apu->mixer_out[(mixer_ptr + i * data_to_fetch / nsamples) % MIXER_BUFFER];
    }
    mixer_ptr = (mixer_ptr + data_to_fetch) % MIXER_BUFFER;

    /*static float t, duty, freq, vol = 0;
    const float t_inc = 1.0 / (audio.freq * audio.channels);
    const float aptr_inc = (float)apu->mixer_ptr / nsamples;
    bool wave_in_progress = false;
    float aptr_max = 0;
    int aptr = 0;
    
    for (int i = 0; i < nsamples; i++) {
        aptr_max += aptr_inc;
        for (int j = aptr; j < aptr_max && j < apu->mixer_ptr; j++, aptr++) {
            pulse_out_t p1_out = apu->pulse1_out[j];
            if (p1_out.clocked) {
                freq = F_CPU_NTSC / (16.0 * (p1_out.period + 1));
                duty = PULSE_DUTY[p1_out.duty];
                t = 0;
            }
            vol = p1_out.vol;
        }
        output[i] = freq > 0 ? pulse_harmonic(t, freq, duty, 10) * vol : 0;
        t += t_inc;
    }
    apu->mixer_ptr = 0;*/

    // Harmonic test.
    /*static float t = 0;
    float duty, freq = 0;
    for (int i = 0; i < nsamples; i++) {
        freq = 400;
        duty = 0.5;
        output[i] = pulse_harmonic(t, freq, duty, 10);
        t += 1.0 / (audio.freq * audio.channels);
    }*/
}
