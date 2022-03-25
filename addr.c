#include "cpu.h"

uint16_t word(uint8_t first, uint8_t second) {
    return (second << 8) + first;
}

addr_t addrm_impl(tframe_t *frame, uint8_t *mem, uint8_t *args) {
    return 0; // Address is not used.
}

addr_t addrm_acc(tframe_t *frame, uint8_t *mem, uint8_t *args) {
    return frame->ac;
}

addr_t addrm_zpg(tframe_t *frame, uint8_t *mem, uint8_t *args) {
    return word(args[0], 0);
}

addr_t addrm_zpgx(tframe_t *frame, uint8_t *mem, uint8_t *args) {
    return (args[0] + frame->x) % 256;
}

addr_t addrm_zpgy(tframe_t *frame, uint8_t *mem, uint8_t *args) {
    return (args[0] + frame->y) % 256;
}

addr_t addrm_abs(tframe_t *frame, uint8_t *mem, uint8_t *args) {
    return word(args[0], args[1]);
}

addr_t addrm_absx(tframe_t *frame, uint8_t *mem, uint8_t *args) {
    return word(args[0], args[1]) + frame->x;
}

addr_t addrm_absy(tframe_t *frame, uint8_t *mem, uint8_t *args) {
    return word(args[0], args[1]) + frame->y;
}

addr_t addrm_rel(tframe_t *frame, uint8_t *mem, uint8_t *args) {
    return frame->pc + (int8_t)args[0];
}

addr_t addrm_ind(tframe_t *frame, uint8_t *mem, uint8_t *args) {
    uint16_t addr = word(args[0], args[1]);
    return word(mem[addr], mem[addr+1]);
}

addr_t addrm_indx(tframe_t *frame, uint8_t *mem, uint8_t *args) {
    uint8_t *new_args = (uint8_t*)addrm_zpgx(frame, mem, args);
    return addrm_ind(frame, mem, new_args);
}

addr_t addrm_indy(tframe_t *frame, uint8_t *mem, uint8_t *args) {
    uint8_t new_args[2] = { args[0], 0 };
    uint8_t *addr = (uint8_t*)addrm_ind(frame, mem, new_args);
    return addrm_absy(frame, mem, addr);
}

const addrmode_t AM_IMPLIED = { addrm_impl, 0 };
const addrmode_t AM_ACCUMULATOR = { addrm_acc, 1 };

const addrmode_t AM_ZEROPAGE = { addrm_zpg, 1 };
const addrmode_t AM_ZEROPAGE_X = { addrm_zpgx, 1 };
const addrmode_t AM_ZEROPAGE_Y = { addrm_zpgy, 1 };

const addrmode_t AM_ABSOLUTE = { addrm_abs, 2 };
const addrmode_t AM_ABSOLUTE_X = { addrm_absx, 2 };
const addrmode_t AM_ABSOLUTE_X = { addrm_absx, 2 };

const addrmode_t AM_RELATIVE = { addrm_rel, 1 };

const addrmode_t AM_INDIRECT = { addrm_ind, 2 };
const addrmode_t AM_INDIRECT_X = { addrm_indx, 1 };
const addrmode_t AM_INDIRECT_Y = { addrm_indy, 1 };
