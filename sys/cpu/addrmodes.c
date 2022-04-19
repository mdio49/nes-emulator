#include <addrmodes.h>

static const mem_loc_t mem_loc(const addr_t vaddr, const uint8_t *ptr, bool crossed) {
    mem_loc_t ret = { vaddr, (uint8_t *)ptr, crossed };
    return ret;
}

static const mem_loc_t addrm_impl(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    return mem_loc(0, NULL, false); // Address is not used.
}

static const mem_loc_t addrm_acc(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    return mem_loc(0, &frame->ac, false);
}

static const mem_loc_t addrm_imm(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    return mem_loc(0, &args[0], false);
}

static const mem_loc_t addrm_zpg(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    return mem_loc(args[0], NULL, false);
}

static const mem_loc_t addrm_zpgx(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = (args[0] + frame->x) & PAGE_MASK;
    return mem_loc(addr, NULL, false);
}

static const mem_loc_t addrm_zpgy(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = (args[0] + frame->y) & PAGE_MASK;
    return mem_loc(addr, NULL, false);
}

static const mem_loc_t addrm_abs(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = bytes_to_word(args[0], args[1]);
    return mem_loc(addr, NULL, false);
}

static const mem_loc_t addrm_absx(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t start = bytes_to_word(args[0], args[1]);
    addr_t target = start + frame->x;
    return mem_loc(target, NULL, (start & ~PAGE_MASK) != (target & ~PAGE_MASK));
}

static const mem_loc_t addrm_absy(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t start = bytes_to_word(args[0], args[1]);
    addr_t target = start + frame->y;
    return mem_loc(target, NULL, (start & ~PAGE_MASK) != (target & ~PAGE_MASK));
}

static const mem_loc_t addrm_rel(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = frame->pc + (int8_t)args[0];
    return mem_loc(addr, NULL, false);
}

static const mem_loc_t addrm_ind(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = bytes_to_word(args[0], args[1]);
    uint8_t low = as_read(as, addr);
    uint8_t high = as_read(as, (addr & ~PAGE_MASK) | ((addr + 1) & PAGE_MASK));
    addr_t target = bytes_to_word(low, high);
    return mem_loc(target, NULL, false);
}

static const mem_loc_t addrm_indx(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = addrm_zpgx(frame, as, args).vaddr;
    uint8_t args1[2] = { addr & 0xFF, addr >> 8 };
    return addrm_ind(frame, as, args1);
}

static const mem_loc_t addrm_indy(const tframe_t *frame, const addrspace_t *as, const uint8_t *args) {
    addr_t addr = args[0];
    uint8_t low = as_read(as, addr);
    uint8_t high = as_read(as, (addr + 1) & 0xFF);
    uint8_t args1[2] = { low, high };
    return addrm_absy(frame, as, args1);
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
