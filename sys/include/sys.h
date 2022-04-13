#ifndef SYS_H
#define SYS_H

#include <cpu.h>
#include <ppu.h>
#include <prog.h>
#include <stdbool.h>

#define SCREEN_WIDTH    256
#define SCREEN_HEIGHT   240 

typedef struct handlers {

    bool        interrupted;
    bool        running;

    void        (*before_execute)(operation_t ins);
    void        (*after_execute)(operation_t ins);

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

extern cpu_t   *cpu;        // The CPU.
extern ppu_t   *ppu;        // The PPU.
extern prog_t  *curprog;    // The cartridge inserted into the system.

#endif
