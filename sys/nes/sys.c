#include <sys.h>
#include <stdlib.h>

void sys_create(void) {
    sys_t *sys = malloc(sizeof(struct sys));
    sys->cpu = cpu_create();
    sys->ppu = ppu_create();
    sys->prog = NULL;
    return sys;
}

sys_t sys_destroy(sys_t *sys) {
    cpu_destroy(sys->cpu);
    ppu_destroy(sys->ppu);
    free(sys);
}

void sys_poweron(sys_t *sys) {

}

void sys_poweroff(sys_t *sys) {

}

void sys_reset(sys_t *sys) {

}

void sys_runprog(sys_t *sys) {
    cpu_t *cpu = sys->cpu;
    ppu_t *ppu = sys->ppu;
    prog_t *prog = sys->prog;
    
}

