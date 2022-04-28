#include <emu.h>

static void audio_callback(void *udata, uint8_t *stream, int len);

static SDL_AudioSpec audio;
static uint32_t mixer_ptr = 0;

bool init_audio(void) {
    /* Set the audio format */
    audio.freq = 262144;
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

/*void flush_apu(const float *data, int len) {
    SDL_QueueAudio(1, (void*)data, len);
}*/

static void audio_callback(void *udata, uint8_t *stream, int len) {
    //printf("-- %d -> %d\n", mixer_ptr, apu->mixer_ptr);
    for (int i = 0; i < len / 4; i++) {
        if (mixer_ptr == apu->mixer_ptr) {
            stream[i] = 0x00;
        }
        else {
            //printf("%d %f\n", mixer_ptr, apu->mixer_out[mixer_ptr]);
            ((float*)stream)[i] = apu->mixer_out[mixer_ptr];
            mixer_ptr = (mixer_ptr + 1) % MIXER_BUFFER;
        }
    }

    // Sound test
    /*const int SquareWavePeriod = (apu->pulse[0].timer_high << 8) | apu->pulse[0].timer_low;
    if (SquareWavePeriod >= 8) {
        const int QuarterSquareWavePeriod = SquareWavePeriod / 4;
        for(int i = 0; i < len / 4; ++i) {
            ((float*)stream)[i] = ((i / QuarterSquareWavePeriod) % 2) ? 0.1f : -0.1f;
        }
    }
    else {
        memset(stream, 0, len);
    }*/
    //memset(stream, 0, len);
}
