#ifndef APU_H
#define APU_H

#include <stdint.h>
#include <stdbool.h>

#define APU_PULSE1      0x4000
#define APU_PULSE2      0x4004
#define APU_TRIANGLE    0x4008
#define APU_NOISE       0x400C
#define APU_DMC         0x4010
#define APU_STATUS      0x4015
#define APU_FRAME       0x4017

/**
 * @brief A struct that contains data for a pulse channel.
 */
typedef struct pulse {
    union {
        struct {
            unsigned    vol     : 4;    // Volume/envelope.
            unsigned    cons    : 1;    // Constant volume.
            unsigned    loop    : 1;    // Envelope loop / length counter halt.
            unsigned    duty    : 2;    // Duty.
        };
        uint8_t reg0;
    };
    union {
        struct {
            unsigned    shift   : 4;
            unsigned    negate  : 1;
            unsigned    period  : 3;
            unsigned    enabled : 2;
        } sweep;
        uint8_t reg1;
    };
    union {
        uint8_t timer_low;
        uint8_t reg2;
    };
    union {
        struct {
            unsigned    timer_high  : 3;
            unsigned    len_counter : 5;
        };
        uint8_t reg3;
    };
} pulse_t;

/**
 * @brief A struct that contains data for a triangle channel.
 */
typedef struct triangle {
    union {
        struct {
            unsigned    lin_counter_load    : 7;
            unsigned    lin_counter_ctrl    : 1;
        };
        uint8_t reg0;
    };
    uint8_t reg1;
    union {
        uint8_t timer_low;
        uint8_t reg2;
    };
    union {
        struct {
            unsigned    timer_high  : 3;
            unsigned    len_counter : 5;
        };
        uint8_t reg3;
    };
} triangle_t;

/**
 * @brief A struct that contains data for a noise channel.
 */
typedef struct noise {
    union {
        struct {
            unsigned    vol                 : 4;    // Volume/envelope.
            unsigned    cons                : 1;    // Constant volume.
            unsigned    len_counter_halt    : 1;    // Length counter halt.
            unsigned                        : 2;
        };
        uint8_t reg0;
    };
    uint8_t reg1;
    union {
        struct {
            unsigned    period  : 4;    // Noise period.
            unsigned            : 3;
            unsigned    loop    : 1;    // Loop noise.
        };
        uint8_t reg2;
    };
    union {
        struct {
            unsigned                : 3;
            unsigned    len_counter : 5;
        };
        uint8_t reg3;
    };
} noise_t;

/**
 * @brief A struct that contains data for a delta modulation channel (DMC).
 */
typedef struct dmc {
    union {
        struct {
            unsigned    freq    : 4;    // Frequency.
            unsigned            : 2;
            unsigned    loop    : 1;    // Loop.
            unsigned    irq     : 1;    // IRQ enable.
        };
        uint8_t reg0;
    };
    union {
        struct {
            unsigned    load    : 7;    // Load counter.
            unsigned            : 1;
        };
        uint8_t reg1;
    };
    union {
        uint8_t addr;   // Sample address.
        uint8_t reg2;
    };
    union {
        uint8_t length; // Sample length.
        uint8_t reg3;
    };
} dmc_t;

typedef struct envelope {

    bool        start_flag;
    uint8_t     decay_level;
    uint8_t     divider;

} envelope_t;

typedef struct sweep_unit {

    uint8_t     divider;
    bool        reload_flag;

} sweep_unit_t;

/**
 * @brief A struct that contains data for an APU.
 */
typedef struct apu {
    
    /* APU channels */
    pulse_t     pulse[2];   // Pulse channels.
    triangle_t  triangle;   // Triangle channel.
    noise_t     noise;      // Noise channel.
    dmc_t       dmc;        // DMC channel.

    // APU status.
    union apu_status {
        struct {
            unsigned    p1      : 1;    // Pulse channel 1.
            unsigned    p2      : 1;    // Pulse channel 2.
            unsigned    tri     : 1;    // Triangle channel.
            unsigned    noise   : 1;    // Noise channel.
            unsigned    dmc     : 1;    // DMC channel.
            unsigned            : 1;
            unsigned    f_irq   : 1;    // Frame IRQ.
            unsigned    d_irq   : 1;    // DMC IRQ.
        };
        uint8_t value;
    } status;

    envelope_t      envelopes[3];       // Envelopes (pulse1, pulse2 and noise respectively).
    sweep_unit_t    sweep_units[2];     // Sweep units (one for each pulse channel).

    unsigned        qframe      : 2;    // A 4-state flag used to determine the quarter that the current quarter-cycle is in.
    unsigned        sequencer   : 3;    // The current step of the sequencer.
    unsigned                    : 3;

    uint16_t        seq_timers[3];      // Sequencer timers (one for each pulse channel and one for triangle channel).

} apu_t;

/**
 * @brief Creates a new instance of an APU.
 * 
 * @return The APU instance.
 */
apu_t *apu_create(void);

/**
 * @brief Destroys an instance of an APU, freeing any associated resources.
 * 
 * @param apu The APU to destroy.
 */
void apu_destroy(apu_t *apu);

/**
 * @brief Updates the given APU by a specified number of frames.
 * 
 * @param apu The APU to update.
 * @param qcycles The number of quarter-cycles to update by (4 quarter-cycles = 1 frame = 2 CPU cycles).
 */
void apu_update(apu_t *apu, int qcycles);

#endif
