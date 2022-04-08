#ifndef SYS_H
#define SYS_H

#include <cpu.h>
#include <ppu.h>
#include <prog.h>
#include <stdbool.h>

typedef struct sys {

    cpu_t   *cpu;       // The CPU.
    ppu_t   *ppu;       // The PPU.
    prog_t  *prog;      // The cartridge inserted into the system.

    bool    interrupted;

} sys_t;

void sys_create(void);
sys_t sys_destroy(sys_t *sys);

void sys_poweron(sys_t *sys);
void sys_poweroff(sys_t *sys);

void sys_reset(sys_t *sys);

void sys_runprog(sys_t *sys);

#endif
