#ifndef SYS_H
#define SYS_H

#include <apu.h>
#include <cpu.h>
#include <ppu.h>
#include <prog.h>
#include <stdbool.h>

#define F_CPU_NTSC  1789773
#define F_CPU_PAL   1662607

typedef enum tv_sys {
    TV_SYS_NTSC,
    TV_SYS_PAL
} tv_sys_t;

typedef struct handlers {

    /* system flags */
    bool        interrupted;                            // Set if emulation is currently paused.
    bool        running;                                // Set if the system is currently running.

    /* cpu handlers */
    void        (*before_execute)(operation_t ins);     // Run before an instruction is executed.
    void        (*after_execute)(operation_t ins);      // Run after an instruction is executed.

    /* ppu handlers */
    void        (*update_screen)(const char *data);     // Flushes the PPU data to the screen.
    
    /* input handlers */
    uint8_t     (*poll_input_p1)(void);                 // Polls for input for player 1.
    uint8_t     (*poll_input_p2)(void);                 // Polls for input for player 2.

} handlers_t;

/**
 * @brief Initializes the system.
 */
void sys_poweron(void);

/**
 * @brief Deinitializes the system, freeing any resources associated with it.
 */
void sys_poweroff(void);

/**
 * @brief Inserts a program (cartridge) into the system, setting up virtual memory
 * and preparing for it to be run.
 * 
 * @param prog The program to insert.
 */
void sys_insert(prog_t *prog);

/**
 * @brief Resets the system (similar to pressing the reset button on the console).
 */
void sys_reset(void);

/**
 * @brief Runs the NES. This method will loop forever unless sys_poweroff() is
 * called via a handler or interrupt in the emulator (or curprog is set to NULL).
 * 
 * @param handlers A struct that contains emulator specific events that take place
 * and variables that may be altered during an interrupt in the emulator.
 */
void sys_run(handlers_t *handlers);

extern apu_t    *apu;                   // The APU.
extern cpu_t    *cpu;                   // The CPU.
extern ppu_t    *ppu;                   // The PPU.
extern prog_t   *curprog;               // The cartridge inserted into the system.
extern tv_sys_t tv_sys;                 // TV system used.

#endif
