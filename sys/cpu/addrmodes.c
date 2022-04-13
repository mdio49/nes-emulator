#include <addrmodes.h>

static const mem_loc_t mem_loc(const addr_t vaddr, const uint8_t *ptr) {
    mem_loc_t ret = { vaddr, (uint8_t *)ptr };
    return ret;
}

static const mem_loc_t addrm_impl(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    return mem_loc(0, NULL); // Address is not used.
}

static const mem_loc_t addrm_acc(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    return mem_loc(0, &frame->ac);
}

static const mem_loc_t addrm_imm(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    return mem_loc(0, &args[0]);
}

static const mem_loc_t addrm_zpg(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    return mem_loc(args[0], NULL);
}

static const mem_loc_t addrm_zpgx(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = (args[0] + frame->x) % 256;
    return mem_loc(addr, NULL);
}

static const mem_loc_t addrm_zpgy(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = (args[0] + frame->y) % 256;
    return mem_loc(addr, NULL);
}

static const mem_loc_t addrm_abs(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = bytes_to_word(args[0], args[1]);
    return mem_loc(addr, NULL);
}

static const mem_loc_t addrm_absx(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = bytes_to_word(args[0], args[1]) + frame->x;
    return mem_loc(addr, NULL);
}

static const mem_loc_t addrm_absy(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = bytes_to_word(args[0], args[1]) + frame->y;
    return mem_loc(addr, NULL);
}

static const mem_loc_t addrm_rel(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = frame->pc + (int8_t)args[0];
    return mem_loc(addr, NULL);
}

static const mem_loc_t addrm_ind(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    uint16_t addr = bytes_to_word(args[0], args[1]);
    uint8_t low = as_read(as, addr);
    uint8_t high = as_read(as, addr + 1);
    addr_t target = bytes_to_word(low, high);
    return mem_loc(target, NULL);
}

static const mem_loc_t addrm_indx(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = addrm_zpgx(frame, as, args).vaddr;
    uint8_t args1[2] = { addr & 0xFF, addr >> 8 };
    return addrm_ind(frame, as, args1);
}

static const mem_loc_t addrm_indy(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    uint8_t args1[2] = { args[0], 0 };
    addr_t addr = addrm_ind(frame, as, args1).vaddr;
    uint8_t args2[2] = { addr & 0xFF, addr >> 8 };
    return addrm_absy(frame, as, args2);
}

const addrmode_t AM_IMPLIED = { addrm_impl, 0 };
const addrmode_t AM_ACCUMULATOR = { addrm_acc, 0 };
const addrmode_t AM_IMMEDIATE = { addrm_imm, 1 };

const addrmode_t AM_ZEROPAGE = { addrm_zpg, 1 };
const addrmode_t AM_ZEROPAGE_X = { addrm_zpgx, 1 };
const addrmode_t AM_ZEROPAGE_Y = { addrm_zpgy, 1 };

const addrmode_t AM_ABSOLUTE = { addrm_abs, 2 };
const addrmode_t AM_ABSOLUTE_X = { addrm_absx, 2 };
const addrmode_t AM_ABSOLUTE_Y = { addrm_absy, 2 };

const addrmode_t AM_RELATIVE = { addrm_rel, 1 };

const addrmode_t AM_INDIRECT = { addrm_ind, 2 };
const addrmode_t AM_INDIRECT_X = { addrm_indx, 1 };
const addrmode_t AM_INDIRECT_Y = { addrm_indy, 1 };