#include <apu.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * @brief Clocks an envelope.
 * 
 * @param env The envelope to clock.
 * @param vol The volume parameter.

 * @param loop The loop flag.
 * @return The envelope's output. 
 */
static inline void envelope_clock(envelope_t *env, uint8_t vol, uint8_t loop);

/**
 * @brief Gets the output of the given envelope.
 * 
 * @param env The envelope.
 * @param vol The volume parameter.
 * @param cons The constant volume flag.
 * @return The envelope's output. 
 */
static inline uint8_t envelope_out(envelope_t *env, uint8_t vol, uint8_t cons);

static const uint8_t PULSE_DUTY[4] = {
    0x01, 0x03, 0x0F, 0xFC
};

static const uint8_t LENGTH_TABLE[32] = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static inline void load_counter(uint8_t *counter, uint8_t load, bool status) {
    if (status && *counter == 0) {
        *counter = LENGTH_TABLE[load];
    }
    else {
        *counter = 0;
    }
}

apu_t *apu_create(void) {
    apu_t *apu = malloc(sizeof(struct apu));
    return apu;
}

void apu_destroy(apu_t *apu) {
    free(apu);
}

void apu_update(apu_t *apu, int hcycles) {
    // Update length counter when the appropriate register is loaded with a value.
    if (!apu->status.p1) {
        apu->pulse[0].len_counter = 0;
    }
    else if (apu->pulse[0].len_counter_load > 0) {
        apu->pulse[0].len_counter = LENGTH_TABLE[apu->pulse[0].len_counter_load];
        apu->pulse[0].len_counter_load = 0;
    }
    if (!apu->status.p2) {
        apu->pulse[1].len_counter = 0;
    }
    else if (apu->pulse[1].len_counter_load > 0) {
        apu->pulse[1].len_counter = LENGTH_TABLE[apu->pulse[1].len_counter_load];
        apu->pulse[1].len_counter_load = 0;
    }

    // Update pulse channel sweep units.
    uint16_t target[2];
    bool sweep_mute[2] = { false, false };
    for (int i = 0; i < 2; i++) {
        pulse_t *pulse = &apu->pulse[i];
        int16_t period = (pulse->timer_high << 8) | pulse->timer_low;
        int16_t shift = period >> pulse->sweep.shift;
        if (pulse->sweep.negate) {
            shift = (i == 0) ? ~shift : -shift; // Pulse 1 uses one's complement.
        }
        target[i] = period + shift;
        if (period < 8 || target[i] > 0x7FF) {
            sweep_mute[i] = true; // Silence the channel.
        }
    }
    
    // Handle sequencer events.
    apu->frame_counter += hcycles;
    if (apu->step == 3 && !apu->irq_occurred && apu->frame_counter >= 2 * QUARTER_FRAME) {
        apu->irq_flag = true;
        apu->irq_occurred = true;
    }
    if (apu->frame_counter >= 2 * QUARTER_FRAME + 1) {
        // Update pulse channels.
        for (int i = 0; i < 2; i++) {
            pulse_t *pulse = &apu->pulse[i];

            // Clock envelope.
            envelope_clock(&pulse->envelope, pulse->vol, apu->pulse->duty);

            // Clock every half-frame.
            if (apu->step == 2 || apu->step == 4) {
                // Adjust the period of the pulse if applicable if the sweep unit is not currently silencing the channel.
                if (!sweep_mute[i] && pulse->sweep_u.divider == 0 && pulse->sweep.enabled && pulse->sweep.shift != 0) {
                    pulse->timer_high = (target[i] & 0x300) >> 8;
                    pulse->timer_low = target[i] & 0x0FF;
                }

                // Clock the sweep unit divider.
                if (pulse->sweep_u.divider == 0 || pulse->sweep_u.reload_flag) {
                    pulse->sweep_u.divider = pulse->sweep.period;
                    pulse->sweep_u.reload_flag = false;
                }
                else {
                    pulse->sweep_u.divider--;
                }

                // Clock length counter.
                if (!pulse->loop && pulse->len_counter > 0) {
                    pulse->len_counter--;
                }
            }
        }

        apu->frame_counter -= 2 * QUARTER_FRAME;
        if (apu->step == 3) {
            apu->irq_occurred = false;
            apu->step = 0;
        }
        else {
            apu->step++;
        }
    }

    // Produce the waveform.
    hcycles += apu->cyc_carry;
    while (hcycles > 1) {
        // Output of each channel.
        uint8_t pulse1, pulse2; //, triangle, noise, dmc;

        // Pulse channels.
        for (int i = 0; i < 2; i++) {
            uint8_t pulse_out;
            pulse_t *pulse = &apu->pulse[i];

            // Get envelope output.
            pulse_out = envelope_out(&pulse->envelope, pulse->vol, pulse->cons);
            
            // Check if sweep unit is silencing channel.
            if (sweep_mute[i]) {
                pulse_out = 0;
            }

            // Check length counter.
            if (!pulse->loop && pulse->len_counter == 0) {
                pulse_out = 0; // Silence the channel.
            }

            // Update timers.
            if (pulse->timer == 0) {
                pulse->timer = (pulse->timer_high << 8) | pulse->timer_low;
                pulse->sequencer--;
            }
            else {
                pulse->timer--;
            }

            // Get the output pulse to send to the mixer.
            if ((PULSE_DUTY[pulse->duty] & (1 << pulse->sequencer)) == 0) {
                pulse_out = 0;
            }

            if (i == 0) {
                pulse1 = pulse_out;
            }
            else {
                pulse2 = pulse_out;
            }
        }

        float pulse_out = pulse1 + pulse2 > 0 ? 95.88 / ((8128 / (pulse1 + pulse2)) + 100) : 0;
        apu->mixer_out[apu->mixer_ptr] = pulse_out;
        apu->mixer_ptr = (apu->mixer_ptr + 1) % MIXER_BUFFER;

        hcycles -= 2;
    }

    if (hcycles == 1) {
        apu->cyc_carry = 1;
    }
}

static inline void envelope_clock(envelope_t *env, uint8_t vol, uint8_t loop) {
    if (env->start_flag) {
        env->decay_level = 15;
        env->divider = vol;
        env->start_flag = false;
    }
    else if (env->divider == 0) {
        env->divider = vol;
        if (env->decay_level > 0) {
            env->decay_level--;
        }
        else if (loop) {
            env->decay_level = 15;
        }
    }
    else {
        env->divider--;
    }
}

static inline uint8_t envelope_out(envelope_t *env, uint8_t vol, uint8_t cons) {
    // If using constant volume, then envelope output is volume; otherwise, it is the decay level of the envelope.
    return cons ? vol : env->decay_level;
}