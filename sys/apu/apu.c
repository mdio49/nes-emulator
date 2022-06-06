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
static inline uint8_t envelope_out(const envelope_t *env, uint8_t vol, uint8_t cons);

/**
 * @brief Reloads a length counter.
 * 
 * @param counter The length counter.
 * @param load The load.
 * @param status The status of the channel.
 * @param reload The reload flag.
 */
static inline void len_counter_load(uint8_t *counter, uint8_t load, bool status, bool reload);

/**
 * @brief Clocks a length counter.
 * 
 * @param counter The length counter.
 * @param halt The halt flag.
 */
static inline void len_counter_clock(uint8_t *counter, bool halt);

static const uint8_t PULSE_DUTY[4] = {
    0x01, 0x03, 0x0F, 0xFC
};

static const uint8_t LENGTH_TABLE[32] = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static const uint16_t NOISE_PERIODS[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

static const uint16_t DMC_RATES[16] = {
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54
};

apu_t *apu_create(void) {
    // Create the APU.
    apu_t *apu = malloc(sizeof(struct apu));

    // Clear registers.
    apu->pulse[0].reg0 = 0;
    apu->pulse[0].reg1 = 0;
    apu->pulse[0].reg2 = 0;
    apu->pulse[0].reg3 = 0;

    apu->pulse[1].reg0 = 0;
    apu->pulse[1].reg1 = 0;
    apu->pulse[1].reg2 = 0;
    apu->pulse[1].reg3 = 0;

    apu->triangle.reg0 = 0;
    apu->triangle.reg1 = 0;
    apu->triangle.reg2 = 0;
    apu->triangle.reg3 = 0;

    apu->noise.reg0 = 0;
    apu->noise.reg1 = 0;
    apu->noise.reg2 = 0;
    apu->noise.reg3 = 0;

    apu->dmc.reg0 = 0;
    apu->dmc.reg1 = 0;
    apu->dmc.reg2 = 0;
    apu->dmc.reg3 = 0;

    apu->status.value = 0;
    apu->frame.value = 0;
    
    // Initialize frame counter.
    apu->frame_counter = 0;
    apu->step = 0;

    // Initialize triangle sequencer.
    apu->triangle.sequencer = 15;
    apu->triangle.desc = true;
    
    // Initialize noise shift register.
    apu->noise.shift_register = 1;

    // Initialize DMC.
    apu->dmc.output = 0;
    apu->dmc.bits_remaining = 1;

    // Generate lookup tables.
    apu->pulse_table[0] = 0;
    for (int i = 1; i <= 30; i++) {
        apu->pulse_table[i] = 95.88 / ((8128.0 / i) + 100.0);
    }
    apu->tnd_table[0][0][0] = 0;
    for (int t = 0; t < 16; t++) {
        for (int n = 0; n < 16; n++) {  
            for (int d = 0; d < 128; d++) {
                if (t == 0 && n == 0 && d == 0)
                    continue;
                apu->tnd_table[t][n][d] = 159.79 / ((1.0 / ((t / 8227.0) + (n / 12241.0) + (d / 22638.0))) + 100.0);
            }
        }
    }

    return apu;
}

void apu_destroy(apu_t *apu) {
    free(apu);
}

void apu_reset(apu_t *apu) {
    // Clear $4015.
    apu->status.value = 0;

    // Reset frame counter.
    apu->frame_counter = 0;
    apu->step = 0;

    // Reset triangle sequencer.
    apu->triangle.sequencer = 15;
    apu->triangle.desc = true;

    // DMC output is ANDed with 1 on reset.
    apu->dmc.output &= 0x01;
}

void apu_update(apu_t *apu, addrspace_t *cpuas, int hcycles) {
    // Update length counter when the appropriate register is loaded with a value.
    len_counter_load(&apu->pulse[0].len_counter, apu->pulse[0].len_counter_load, apu->status.p1, apu->pulse[0].len_counter_reload);
    len_counter_load(&apu->pulse[1].len_counter, apu->pulse[1].len_counter_load, apu->status.p2, apu->pulse[1].len_counter_reload);
    len_counter_load(&apu->triangle.len_counter, apu->triangle.len_counter_load, apu->status.tri, apu->triangle.len_counter_reload);
    len_counter_load(&apu->noise.len_counter, apu->noise.len_counter_load, apu->status.noise, apu->noise.len_counter_reload);

    // Reset the length counter reload flags.
    apu->pulse[0].len_counter_reload = false;
    apu->pulse[1].len_counter_reload = false;
    apu->triangle.len_counter_reload = false;
    apu->noise.len_counter_reload = false;

    // Reload DMC output if necessary.
    if (apu->dmc.output_reload) {
        apu->dmc.output = apu->dmc.load;
        apu->dmc.output_reload = false;
    }

    // If DMC is disabled, then set bytes remaining to 0.
    if (!apu->status.dmc) {
        apu->dmc.bytes_remaining = 0;
    }

    // If the DMC start flag is set, then restart the sample.
    if (apu->dmc.start_flag) {
        if (apu->dmc.bytes_remaining == 0) {
            apu->dmc.addr_counter = 0xC000 | (apu->dmc.addr << 6);
            apu->dmc.bytes_remaining = apu->dmc.length * 16 + 1;
        }
        apu->dmc.start_flag = false;
    }

    // Clear DMC IRQ flag if IRQ is disabled.
    if (!apu->dmc.irq) {
        apu->status.d_irq = false;
    }

    // Check if the frame counter should be reset.
    if (apu->frame_reset > 0) {
        if (hcycles >= apu->frame_reset) {
            if (apu->frame.mode == 1) {
                // Clock the half-frame and quarter-frame events if the mode is set to 5-step sequence.
                apu->frame_counter = 2 * (QUARTER_FRAME - 2) - hcycles + 2; // Frame counter should be 0 after the update.
                apu->step = 4;
            }
            else {
                apu->frame_counter = -hcycles; // Frame counter should be 0 after the update.
                apu->step = 0;
            }
            apu->frame_reset = 0;
        }
        else {
            apu->frame_reset -= hcycles;
        }
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

    // Calculate the increment between quarter-frames.
    const int frame_step = apu->step == 4 ? 2 * (QUARTER_FRAME - 2) : apu->step < 2 ? 2 * QUARTER_FRAME : 2 * (QUARTER_FRAME + 1);
    
    // Handle sequencer events.
    apu->frame_counter += hcycles;
    if (apu->frame.mode == 0 && apu->step == 3 && apu->frame_counter >= frame_step && !apu->irq_occurred) {
        if (apu->frame.irq) {
            apu->status.f_irq = 0;
        }
        else {
            apu->status.f_irq = 1;
            apu->irq_flag = true;
        }
        apu->irq_occurred = true;
    }
    if (apu->frame_counter > frame_step) {
        // Check if the current step is a half-frame.
        bool half_frame = apu->step == 1 || (apu->frame.mode == 0 && apu->step == 3) || (apu->frame.mode == 1 && apu->step == 4);

        // Don't clock on the 4th step in 5-step mode.
        if (apu->frame.mode == 0 || apu->step != 3) {
            /* Update pulse channels. */
            for (int i = 0; i < 2; i++) {
                pulse_t *pulse = &apu->pulse[i];

                // Clock envelope.
                envelope_clock(&pulse->envelope, pulse->vol, pulse->loop);

                // Clock every half-frame.
                if (half_frame) {
                    // Adjust the period of the pulse if applicable if the sweep unit is not currently silencing the channel.
                    if (!sweep_mute[i] && pulse->sweep_u.divider == 0 && pulse->sweep.enabled && pulse->sweep.shift != 0) {
                        pulse->timer_high = (target[i] & 0x700) >> 8;
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
                    len_counter_clock(&pulse->len_counter, pulse->loop);
                }
            }

            /* Update triangle channel. */

            // Clock linear counter.
            if (apu->triangle.lin_counter_reload) {
                apu->triangle.lin_counter = apu->triangle.lin_counter_load;
            }
            else if (apu->triangle.lin_counter > 0) {
                apu->triangle.lin_counter--;
            }
            if (!apu->triangle.loop) {
                apu->triangle.lin_counter_reload = false;
            }

            // Clock length counter (every half-frame).
            if (half_frame) {
                len_counter_clock(&apu->triangle.len_counter, apu->triangle.loop);
            }

            /* Update noise channel. */

            // Clock envelope.
            envelope_clock(&apu->noise.envelope, apu->noise.vol, apu->noise.loop);

            // Clock length counter.
            if (half_frame) {
                len_counter_clock(&apu->noise.len_counter, apu->noise.loop);
            }
        }
        
        // Decrement the frame counter and increment the step.
        apu->frame_counter -= frame_step;
        if ((apu->frame.mode == 0 && apu->step == 3) || (apu->frame.mode == 1 && apu->step == 4)) {
            apu->irq_occurred = false;
            apu->step = 0;

            // Realign frame counter for next frame.
            apu->frame_counter -= 2;
        }
        else {
            apu->step++;
        }
    }

    // Get the number of APU cycles to process.
    int cycles = (hcycles + apu->cyc_carry) / 2;
    apu->cyc_carry = (hcycles + apu->cyc_carry) % 2;

    // This clocks at the rate of the CPU clock.
    while (hcycles > 0) {
        // Update triangle timer.
        if (apu->triangle.timer == 0) {
            // Reload timer.
            apu->triangle.timer = (apu->triangle.timer_high << 8) | apu->triangle.timer_low;

            // Clock the sequencer if both the length counter and linear counter are non-zero.
            if (apu->triangle.len_counter > 0 && apu->triangle.lin_counter > 0) {
                if (apu->triangle.sequencer == 0 && apu->triangle.desc) {
                    apu->triangle.desc = false;
                }
                else if (apu->triangle.sequencer == 15 && !apu->triangle.desc) {
                    apu->triangle.desc = true;
                }
                else {
                    apu->triangle.sequencer += apu->triangle.desc ? -1 : 1;
                }
            }
        }
        else {
            apu->triangle.timer--;
        }

        hcycles--;
    }    

    // This clocks at the rate of the APU clock.
    while (cycles > 0) {
        // Output of each channel.
        uint8_t pulse1, pulse2, triangle, noise, dmc;

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

        // Get output of triangle channel.
        if (apu->triangle.len_counter > 0 && apu->triangle.lin_counter > 0) {
            triangle = apu->triangle.sequencer;
        }
        else {
            triangle = 0;
        }

        // Ignore ultrasonic frequencies (i.e. period less than 2).
        /*if (apu->triangle.timer_low < 2 && apu->triangle.timer_high == 0) {
            triangle = 0;
        }*/

        // Update noise timer.
        if (apu->noise.timer == 0) {
            // Calculate feedback.
            uint8_t bit0 = apu->noise.shift_register & 0x01;
            uint8_t bit1 = (apu->noise.shift_register >> (apu->noise.mode ? 6 : 1)) & 0x01;
            uint8_t feedback = bit0 ^ bit1;

            // Update shift register.
            apu->noise.shift_register >>= 1;
            apu->noise.shift_register |= feedback << 14;

            // Reload timer.
            apu->noise.timer = NOISE_PERIODS[apu->noise.period];
        }
        else {
            apu->noise.timer--;
        }

        // Get output of noise channel.
        if ((apu->noise.shift_register & 0x01) == 0 && apu->noise.len_counter > 0) {
            noise = envelope_out(&apu->noise.envelope, apu->noise.vol, apu->noise.cons);
        }
        else {
            noise = 0;
        }

        // Update DMC timer.
        //bool dmc_sample_ended = false;
        if (apu->dmc.timer == 0) {
            // Reload the timer.
            apu->dmc.timer = DMC_RATES[apu->dmc.rate] / 2;

            // Change output level if silence flag is clear.
            if (!apu->dmc.silence) {
                uint8_t delta = apu->dmc.shift_register & 0x01;
                if (delta > 0 && apu->dmc.output <= 125) {
                    apu->dmc.output += 2;
                }
                else if (delta == 0 && apu->dmc.output >= 2) {
                    apu->dmc.output -= 2;
                }
            }

            // Clock the shift register and decrement the bits remaining.
            apu->dmc.shift_register >>= 1;
            apu->dmc.bits_remaining--;

            // If there are no bits remaining in the shift register, then load the next sample.
            if (apu->dmc.bits_remaining == 0) {
                if (apu->dmc.bytes_remaining > 0) {
                    // Clear the silence flag.
                    apu->dmc.silence = false;

                    // Load next sample byte.
                    apu->dmc.shift_register = as_read(cpuas, apu->dmc.addr_counter);
                    apu->dmc.bytes_remaining--;

                    // Increment address counter.
                    if (apu->dmc.addr_counter == 0xFFFF) {
                        apu->dmc.addr_counter = 0x8000;
                    }
                    else {
                        apu->dmc.addr_counter++;
                    }

                    // Check if sample buffer is empty.
                    if (apu->dmc.bytes_remaining == 0) {
                        // Restart the channel if the loop flag is set.
                        if (apu->dmc.loop) {
                            apu->dmc.addr_counter = 0xC000 | (apu->dmc.addr << 6);
                            apu->dmc.bytes_remaining = apu->dmc.length * 16 + 1;
                        }
                        else if (apu->dmc.irq) {
                            // Generate IRQ if enabled and if not looping.
                            apu->status.d_irq = true;
                            apu->irq_flag = true;
                        }
                    }
                }
                else if (!apu->dmc.silence) {
                    // Silence the channel if the sample buffer is empty (or if $4015 was cleared).
                    apu->dmc.silence = true;
                    //dmc_sample_ended = true;
                }
                apu->dmc.bits_remaining = 8;
            }
        }
        else {
            apu->dmc.timer--;
        }

        // Get output of DMC.
        dmc = !apu->dmc.silence ? apu->dmc.output : 0;

        // Clear the DMC output if the sample buffer just ended.
        /*if (dmc_sample_ended) {
            apu->dmc.output = 0;
        }*/

        // Get mixer output.
        float pulse_out = apu->pulse_table[pulse1 + pulse2];
        float tnd_out = apu->tnd_table[triangle][noise][dmc];

        // Block thread while producer pointer is too far ahead.
        int delta;
        do {
            delta = (MIXER_BUFFER + apu->out.prod - apu->out.cons) % MIXER_BUFFER;
        }
        while (delta > MIXER_MAX_DELTA);

        // Place output into buffer.
        apu->out.buffer[apu->out.prod] = pulse_out + tnd_out;
        apu->out.prod = (apu->out.prod + 1) % MIXER_BUFFER;

        cycles--;
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

static inline uint8_t envelope_out(const envelope_t *env, uint8_t vol, uint8_t cons) {
    // If using constant volume, then envelope output is volume; otherwise, it is the decay level of the envelope.
    return cons ? vol : env->decay_level;
}

static inline void len_counter_load(uint8_t *counter, uint8_t load, bool status, bool reload) {
    if (!status) {
        *counter = 0; // If the channel is disabled, then force the length counter to 0.
    }
    else if (reload) {
        *counter = LENGTH_TABLE[load]; // Otherwise, if the reload flag is set, load it with a value from the table.
    }
}

static inline void len_counter_clock(uint8_t *counter, bool halt) {
    if (!halt && *counter > 0) {
        (*counter)--;
    }
}
