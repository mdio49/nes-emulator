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

#define QUARTER_FRAME   3728

#define MIXER_BUFFER    65536

typedef struct envelope {

    bool        start_flag;
    unsigned    decay_level : 4;
    unsigned    divider     : 4;

} envelope_t;

typedef struct sweep_unit {

    uint8_t     divider;        // The divider.
    bool        reload_flag;    // The reload flag.

} sweep_unit_t;

/**
 * @brief A struct that contains data for a pulse channel.
 */
typedef struct pulse {
    /* registers */
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
            unsigned    timer_high          : 3;
            unsigned    len_counter_load    : 5;
        };
        uint8_t reg3;
    };

    /* units */
    
    envelope_t      envelope;   // Envelope unit.
    sweep_unit_t    sweep_u;    // Sweep unit.

    /* other variables */

    uint8_t         len_counter;
    unsigned        sequencer           : 3;    // The sequencer.
    unsigned        timer               : 11;   // The sequencer's timer/divider.
    unsigned        len_counter_reload  : 1;    // Set if the length counter should be reloaded.
    unsigned                            : 1;
    
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

    /* other variables */
    uint16_t        frame_counter;          // The frame counter.
    unsigned        step            : 3;    // The current sequencer step.
    unsigned        cyc_carry       : 1;    // Set if the last update had one half-cycle left over.
    unsigned        irq_occurred    : 1;    // Set if an IRQ has already occurred for the current frame.
    unsigned        irq_flag        : 1;    // Set if an IRQ should occur.
    unsigned                        : 2;

    float           mixer_out[MIXER_BUFFER];
    uint32_t        mixer_ptr;

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
 * @param hcycles The number of half-cycles that have passed (2 CPU cycles = 1 APU cycle).
 */
void apu_update(apu_t *apu, int hcycles);

#endif
