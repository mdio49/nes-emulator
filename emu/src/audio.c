#include <emu.h>

/* SDL audio callback function */
static void audio_callback(void *udata, uint8_t *stream, int len);

/* SDL audio specifications */
static SDL_AudioSpec audio;

static bool muted = false;

bool init_audio(void) {
    /* Set the audio format */
    audio.freq = 192000;
    audio.format = AUDIO_F32;
    audio.channels = 2;
    audio.samples = 1024;
    audio.callback = audio_callback;
    audio.userdata = NULL;

    if (SDL_OpenAudio(&audio, NULL) != 0) {
        printf("Couldn't open audio: %s\n", SDL_GetError());
        return false;
    }

    SDL_PauseAudio(0);
    return true;
}

void free_audio(void) {
    // Close the audio device.
    SDL_CloseAudio();
}

void toggle_audio(void) {
    muted = !muted;
}

bool is_muted(void) {
    return muted;
}

static void audio_callback(void *udata, uint8_t *stream, int len) {
    float *output = (float*)stream;

    // Determine the amount of data to fetch.
    const int nsamples = len / sizeof(float);
    const int cpu_freq = tv_sys == TV_SYS_PAL ? F_CPU_PAL : F_CPU_NTSC;
    const double out_per_sec = cpu_freq / 2.0;
    const double calls_per_sec = (double)(audio.freq * audio.channels) / nsamples;
    const int data_to_fetch = out_per_sec / calls_per_sec;

    // Determine the maximum amount of data available in the buffer.
    const int max_data = (MIXER_BUFFER + apu->out.prod - apu->out.cons) % MIXER_BUFFER;

    // Fill the output buffer based on the samples produced by the APU.
    for (int i = 0; i < nsamples; i++) {
        int offset = i * data_to_fetch / nsamples;
        output[i] = offset < max_data && !muted ? apu->out.buffer[(apu->out.cons + offset) % MIXER_BUFFER] : 0;
    }

    // Increment consumer pointer based on the amount of data collected.
    apu->out.cons = (apu->out.cons + min(max_data, data_to_fetch)) % MIXER_BUFFER;
}
