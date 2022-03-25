#include "cpu.h"

static uint16_t word(uint8_t first, uint8_t second) {
    return (second << 8) + first;
}

static addr_t addrm_impl(const tframe_t *frame, const uint8_t *mem, const uint8_t *args) {
    return 0; // Address is not used.
}

static addr_t addrm_acc(const tframe_t *frame, const uint8_t *mem, const uint8_t *args) {
    return frame->ac;
}

static addr_t addrm_zpg(const tframe_t *frame, const uint8_t *mem, const uint8_t *args) {
    return word(args[0], 0);
}

static addr_t addrm_zpgx(const tframe_t *frame, const uint8_t *mem, const uint8_t *args) {
    return (args[0] + frame->x) % 256;
}

static addr_t addrm_zpgy(const tframe_t *frame, const uint8_t *mem, const uint8_t *args) {
    return (args[0] + frame->y) % 256;
}

static addr_t addrm_abs(const tframe_t *frame, const uint8_t *mem, const uint8_t *args) {
    return word(args[0], args[1]);
}

static addr_t addrm_absx(const tframe_t *frame, const uint8_t *mem, const uint8_t *args) {
    return word(args[0], args[1]) + frame->x;
}

static addr_t addrm_absy(const tframe_t *frame, const uint8_t *mem, const uint8_t *args) {
    return word(args[0], args[1]) + frame->y;
}

static addr_t addrm_rel(const tframe_t *frame, const uint8_t *mem, const uint8_t *args) {
    return frame->pc + (int8_t)args[0];
}

static addr_t addrm_ind(const tframe_t *frame, const uint8_t *mem, const uint8_t *args) {
    uint16_t addr = word(args[0], args[1]);
    return word(mem[addr], mem[addr+1]);
}

static addr_t addrm_indx(const tframe_t *frame, const uint8_t *mem, const uint8_t *args) {
    addr_t addr = addrm_zpgx(frame, mem, args);
    uint8_t args1[2] = { (uint8_t)(addr & (1 << 8)), (uint8_t)(addr >> 8) };
    return addrm_ind(frame, mem, args1);
}

static addr_t addrm_indy(const tframe_t *frame, const uint8_t *mem, const uint8_t *args) {
    uint8_t args1[2] = { args[0], 0 };
    addr_t addr = addrm_ind(frame, mem, args1);
    uint8_t args2[2] = { (uint8_t)(addr & (1 << 8)), (uint8_t)(addr >> 8) };
    return addrm_absy(frame, mem, args2);
}

const addrmode_t AM_IMPLIED = { addrm_impl, 0 };
const addrmode_t AM_ACCUMULATOR = { addrm_acc, 1 };

const addrmode_t AM_ZEROPAGE = { addrm_zpg, 1 };
const addrmode_t AM_ZEROPAGE_X = { addrm_zpgx, 1 };
const addrmode_t AM_ZEROPAGE_Y = { addrm_zpgy, 1 };

const addrmode_t AM_ABSOLUTE = { addrm_abs, 2 };
const addrmode_t AM_ABSOLUTE_X = { addrm_absx, 2 };
const addrmode_t AM_ABSOLUTE_Y = { addrm_absx, 2 };

const addrmode_t AM_RELATIVE = { addrm_rel, 1 };

const addrmode_t AM_INDIRECT = { addrm_ind, 2 };
const addrmode_t AM_INDIRECT_X = { addrm_indx, 1 };
const addrmode_t AM_INDIRECT_Y = { addrm_indy, 1 };
