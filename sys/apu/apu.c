#include <apu.h>
#include <stdlib.h>
#include <stdbool.h>

apu_t *apu_create(void) {
    apu_t *apu = malloc(sizeof(struct apu));
    return apu;
}

void apu_destroy(apu_t *apu) {
    free(apu);
}

void apu_cycle(apu_t *apu, int qcycles) {
    while (qcycles > 0) {
        uint8_t pulse1, pulse2, triangle, noise, dmc;
        uint8_t env_ouput[3];

        uint8_t vol[3] = { apu->pulse[0].vol, apu->pulse[1].vol, apu->noise.vol };
        uint8_t cons[3] = { apu->pulse[0].cons, apu->pulse[1].cons, apu->noise.cons };
        uint8_t loop[3] = { apu->pulse[0].loop, apu->pulse[1].loop, apu->noise.loop };

        // Update envelopes.
        for (int i = 0; i < 3; i++) {
            envelope_t *envelope = &apu->envelopes[i];
            if (envelope->start_flag) {
                envelope->decay_level = 15;
                envelope->divider = vol[i];
                envelope->start_flag = false;
            }
            else if (envelope->divider == 0) {
                envelope->divider = vol[i];
                if (envelope->decay_level > 0) {
                    envelope->decay_level--;
                }
                else if (loop[i]) {
                    envelope->decay_level = 15;
                }
            }
            else {
                envelope->divider--;
            }

            // If using constant volume, then envelope output is value in volume
            // register; otherwise, it is the decay level of the envelope.
            env_ouput[i] = cons[i] ? vol[i] : envelope->decay_level;
        }

        // Updated every half-cycle.
        if ((apu->qframe & 0x01) == 0) {
            // Update sweep units.
            for (int i = 0; i < 2; i++) {
                sweep_unit_t *sweep = &apu->sweep_units[i];
                pulse_t *pulse = &apu->pulse[i];
                
                int16_t period = (pulse->timer_high << 8) | pulse->timer_low;
                int16_t shift = period >> pulse->sweep.shift;
                if (pulse->sweep.negate) {
                    shift = (i == 1) ? ~shift : -shift; // Pulse 1 uses one's complement.
                }
                int16_t target = period + shift;
                if (period < 8 || target > 0x7FF) {
                    // TODO: Mute channel.
                }
                else if (sweep->divider == 0 && pulse->sweep.enabled) {
                    pulse->timer_high = (target & 0x300) >> 8;
                    pulse->timer_low = target & 0x0FF;
                }

                if (sweep->divider == 0 || sweep->reload_flag) {
                    sweep->divider = pulse->sweep.period;
                    sweep->reload_flag = false;
                }
                else {
                    sweep->divider--;
                }
            }

            // Update length counters.
            if (apu->pulse[0].len_counter > 0 && !apu->pulse[0].loop) {
                apu->pulse->len_counter--;
            }
            if (apu->pulse[1].len_counter > 0 && !apu->pulse[1].loop) {
                apu->pulse->len_counter--;
            }
        }

        // Updated every APU cycle (i.e. every 2 CPU cycles).
        if ((apu->qframe & 0x03) == 0) {
            if (apu->sequencer == 0) {
                // Update pulse sequence timers.
                for (int i = 0; i < 2; i++) {
                    if (apu->seq_timers[i] > 0) {
                        apu->seq_timers[i]--;
                    }
                    else {
                        apu->seq_timers[i] = (apu->pulse[i].timer_high << 8) | apu->pulse[i].timer_low;
                        pulse1 = env_ouput[i];
                    }
                }

                // Update triangle sequence timer.
                if (apu->seq_timers[2] > 0) {
                    apu->seq_timers[2]--;
                }
                else {
                    apu->seq_timers[2] = (apu->triangle.timer_high << 8) | apu->triangle.timer_low;
                    triangle = env_ouput[2];
                }

                apu->sequencer = 7;
            }
            else {
                apu->sequencer--;
            }
        }

        apu->qframe++;
        qcycles--;
    }
}
