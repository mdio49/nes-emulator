#include "cpu.h"

void sta_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    frame->ac = mem[addr];
}

void sta_apply_immediate(tframe_t *frame, uint8_t *mem, uint8_t value) {
    frame->ac = value;
}

const instruction_t INS_STA = { sta_apply, sta_apply_immediate };
