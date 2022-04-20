#include <addrmodes.h>
#include <instructions.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Helper functions.
 */

static inline int def_cycles(const addrmode_t *am, mem_loc_t loc) {
    int cycles = 0;
    if (am == &AM_IMPLIED || am == &AM_IMMEDIATE)
        cycles = 2;
    else if (am == &AM_ZEROPAGE)
        cycles = 3;
    else if (am == &AM_ABSOLUTE || am == &AM_ZEROPAGE_X || am == &AM_ZEROPAGE_Y)
        cycles = 4;
    else if (am == &AM_ABSOLUTE_X || am == &AM_ABSOLUTE_Y)
        cycles = 4 + loc.page_boundary_crossed;
    else if (am == &AM_INDIRECT_X)
        cycles = 6;
    else if (am == &AM_INDIRECT_Y)
        cycles = 5 + loc.page_boundary_crossed;
    return cycles;
}

static inline void update_sign_flags(tframe_t *frame, uint8_t result) {
    frame->sr.zero = (result == 0x00);
    frame->sr.neg = ((result & 0x80) == 0x80);
}

static uint8_t load(const addrspace_t *as, mem_loc_t loc) {
    uint8_t value;
    if (loc.ptr != NULL) {
        value = *loc.ptr;
    }
    else {
        value = as_read(as, loc.vaddr);
    }
    return value;
}

static uint8_t store(const addrspace_t *as, mem_loc_t loc, uint8_t value) {
    if (loc.ptr != NULL) {
        *loc.ptr = value;
    }
    else {
        as_write(as, loc.vaddr, value);
    }
    return value;
}

/**
 * Transfer instructions.
 */

static int lda_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->ac = load(as, loc);
    update_sign_flags(frame, frame->ac);
    return def_cycles(am, loc);
}

static int ldx_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->x = load(as, loc);
    update_sign_flags(frame, frame->x);
    return def_cycles(am, loc);
}

static int ldy_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->y = load(as, loc);
    update_sign_flags(frame, frame->y);
    return def_cycles(am, loc);
}

static int sta_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    store(as, loc, frame->ac);
    loc.page_boundary_crossed = true;
    return def_cycles(am, loc);
}

static int stx_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    store(as, loc, frame->x);
    return def_cycles(am, loc);
}

static int sty_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    store(as, loc, frame->y);
    return def_cycles(am, loc);
}

static int tax_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->x = frame->ac;
    update_sign_flags(frame, frame->x);
    return def_cycles(am, loc);
}

static int tay_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->y = frame->ac;
    update_sign_flags(frame, frame->y);
    return def_cycles(am, loc);
}

static int tsx_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->x = frame->sp;
    update_sign_flags(frame, frame->x);
    return def_cycles(am, loc);
}

static int txa_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->ac = frame->x;
    update_sign_flags(frame, frame->ac);
    return def_cycles(am, loc);
}

static int txs_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->sp = frame->x;
    return def_cycles(am, loc);
}

static int tya_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->ac = frame->y;
    update_sign_flags(frame, frame->ac);
    return def_cycles(am, loc);
}

const instruction_t INS_LDA = { "LDA", lda_apply, false };
const instruction_t INS_LDX = { "LDX", ldx_apply, false };
const instruction_t INS_LDY = { "LDY", ldy_apply, false };

const instruction_t INS_STA = { "STA", sta_apply, false };
const instruction_t INS_STX = { "STX", stx_apply, false };
const instruction_t INS_STY = { "STY", sty_apply, false };

const instruction_t INS_TAX = { "TAX", tax_apply, false };
const instruction_t INS_TAY = { "TAY", tay_apply, false };
const instruction_t INS_TSX = { "TSX", tsx_apply, false };
const instruction_t INS_TXA = { "TXA", txa_apply, false };
const instruction_t INS_TXS = { "TXS", txs_apply, false };
const instruction_t INS_TYA = { "TYA", tya_apply, false };

/**
 * Stack instructions.
 */

static int pha_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    push(frame, as, frame->ac);
    return 3;
}

static int php_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    push(frame, as, frame->sr.bits | SR_BREAK | SR_IGNORED);
    return 3;
}

static int pla_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->ac = pull(frame, as);
    update_sign_flags(frame, frame->ac);
    return 4;
}

static int plp_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t bits = pull(frame, as);
    uint8_t mask = SR_BREAK | SR_IGNORED;
    frame->sr.bits = (bits & ~mask) | (frame->sr.bits & mask);
    return 4;
}

const instruction_t INS_PHA = { "PHA", pha_apply, false };
const instruction_t INS_PHP = { "PHP", php_apply, false };
const instruction_t INS_PLA = { "PLA", pla_apply, false };
const instruction_t INS_PLP = { "PLP", plp_apply, false };

/**
 * Decrements and increments.
 */

static int dec_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    update_sign_flags(frame, store(as, loc, --value));
    loc.page_boundary_crossed = true;
    return def_cycles(am, loc) + 2;
}

static int dex_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    update_sign_flags(frame, --frame->x);
    return def_cycles(am, loc);
}

static int dey_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    update_sign_flags(frame, --frame->y);
    return def_cycles(am, loc);
}

static int inc_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    update_sign_flags(frame, store(as, loc, ++value));
    loc.page_boundary_crossed = true;
    return def_cycles(am, loc) + 2;
}

static int inx_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    update_sign_flags(frame, ++frame->x);
    return def_cycles(am, loc);
}

static int iny_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    update_sign_flags(frame, ++frame->y);
    return def_cycles(am, loc);
}

const instruction_t INS_DEC = { "DEC", dec_apply, false };
const instruction_t INS_DEX = { "DEX", dex_apply, false };
const instruction_t INS_DEY = { "DEY", dey_apply, false };
const instruction_t INS_INC = { "INC", inc_apply, false };
const instruction_t INS_INX = { "INX", inx_apply, false };
const instruction_t INS_INY = { "INY", iny_apply, false };

/**
 * Arithmetic operators.
 */

static uint8_t add(tframe_t *frame, uint8_t arg) {
    uint16_t result = frame->ac + arg + frame->sr.carry;
    frame->sr.carry = (result > 255);
    frame->sr.vflow = ((frame->ac ^ arg) & 0x80) == 0 && ((frame->ac ^ result) & 0x80) != 0;
    return result;
}

static int adc_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    frame->ac = add(frame, value);
    update_sign_flags(frame, frame->ac);
    return def_cycles(am, loc);
}

static int sbc_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    frame->ac = add(frame, ~value);
    update_sign_flags(frame, frame->ac);
    return def_cycles(am, loc);
}

const instruction_t INS_ADC = { "ADC", adc_apply, false };
const instruction_t INS_SBC = { "SBC", sbc_apply, false };

/**
 * Logical operators.
 */

static int and_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    frame->ac = frame->ac & value;
    update_sign_flags(frame, frame->ac);
    return def_cycles(am, loc);
}

static int eor_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    frame->ac = frame->ac ^ value;
    update_sign_flags(frame, frame->ac);
    return def_cycles(am, loc);
}

static int ora_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    frame->ac = frame->ac | value;
    update_sign_flags(frame, frame->ac);
    return def_cycles(am, loc);
}

const instruction_t INS_AND = { "AND", and_apply, false };
const instruction_t INS_EOR = { "EOR", eor_apply, false };
const instruction_t INS_ORA = { "ORA", ora_apply, false };

/**
 * Shift & rotate instructions.
 */

static int asl_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    uint8_t new_val = store(as, loc, value << 1);
    frame->sr.carry = (value & 0x80) > 0;
    update_sign_flags(frame, new_val);
    loc.page_boundary_crossed = true;
    return am == &AM_ACCUMULATOR ? 2 : def_cycles(am, loc) + 2;
}

static int lsr_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    uint8_t new_val = store(as, loc, value >> 1);
    frame->sr.carry = (value & 0x01) > 0;
    update_sign_flags(frame, new_val);
    loc.page_boundary_crossed = true;
    return am == &AM_ACCUMULATOR ? 2 : def_cycles(am, loc) + 2;
}

static int rol_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    uint8_t new_val = store(as, loc, (value << 1) | frame->sr.carry);
    frame->sr.carry = (value & 0x80) > 0;
    update_sign_flags(frame, new_val);
    loc.page_boundary_crossed = true;
    return am == &AM_ACCUMULATOR ? 2 : def_cycles(am, loc) + 2;
}

static int ror_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    uint8_t new_val = store(as, loc, (value >> 1) | (frame->sr.carry << 7));
    frame->sr.carry = (value & 0x01) > 0;
    update_sign_flags(frame, new_val);
    loc.page_boundary_crossed = true;
    return am == &AM_ACCUMULATOR ? 2 : def_cycles(am, loc) + 2;
}

const instruction_t INS_ASL = { "ASL", asl_apply, false };
const instruction_t INS_LSR = { "LSR", lsr_apply, false };
const instruction_t INS_ROL = { "ROL", rol_apply, false };
const instruction_t INS_ROR = { "ROR", ror_apply, false };

/**
 * Flag instructions.
 */

static int clc_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->sr.carry = 0;
    return 2;
}

static int cld_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->sr.dec = 0;
    return 2;
}

static int cli_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->sr.irq = 0;
    return 2;
}

static int clv_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->sr.vflow = 0;
    return 2;
}

static int sec_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->sr.carry = 1;
    return 2;
}

static int sed_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->sr.dec = 1;
    return 2;
}

static int sei_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->sr.irq = 1;
    return 2;
}

const instruction_t INS_CLC = { "CLC", clc_apply, false };
const instruction_t INS_CLD = { "CLD", cld_apply, false };
const instruction_t INS_CLI = { "CLI", cli_apply, false };
const instruction_t INS_CLV = { "CLV", clv_apply, false };
const instruction_t INS_SEC = { "SEC", sec_apply, false };
const instruction_t INS_SED = { "SED", sed_apply, false };
const instruction_t INS_SEI = { "SEI", sei_apply, false };

/**
 * Comparisons.
 */

static void compare(tframe_t *frame, uint8_t reg, uint8_t value) {
    uint8_t result = reg - value;
    frame->sr.carry = (reg >= value);
    update_sign_flags(frame, result);
}

static int cmp_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    compare(frame, frame->ac, value);
    return def_cycles(am, loc);
}

static int cmx_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    compare(frame, frame->x, value);
    return def_cycles(am, loc);
}

static int cmy_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    compare(frame, frame->y, value);
    return def_cycles(am, loc);
}

const instruction_t INS_CMP = { "CMP", cmp_apply, false };
const instruction_t INS_CPX = { "CPX", cmx_apply, false };
const instruction_t INS_CPY = { "CPY", cmy_apply, false };

/**
 * Conditional branch instructions.
 */

static int branch(tframe_t *frame, addr_t target, bool condition) {
    int cycles = 2;
    if (condition) {
        cycles++; // Add 1 cycle if branch occurs.
        if (((frame->pc + 2) & ~PAGE_MASK) != ((target + 2) & ~PAGE_MASK))
            cycles++; // Add 1 cycle if branch occurs on different page.
        frame->pc = target;
    }
    return cycles;
}

// Branch on carry clear.
static int bcc_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    return branch(frame, loc.vaddr, frame->sr.carry == 0);
}

// Branch on carry set.
static int bcs_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    return branch(frame, loc.vaddr, frame->sr.carry == 1);
}

// Branch on result zero.
static int beq_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    return branch(frame, loc.vaddr, frame->sr.zero == 1);
}

// Branch on result minus (negative).
static int bmi_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    return branch(frame, loc.vaddr, frame->sr.neg == 1);
}

// Branch on result not zero.
static int bne_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    return branch(frame, loc.vaddr, frame->sr.zero == 0);
}

// Branch on result plus (positive).
static int bpl_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    return branch(frame, loc.vaddr, frame->sr.neg == 0);
}

// Branch on overflow clear.
static int bvc_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    return branch(frame, loc.vaddr, frame->sr.vflow == 0);
}

// Branch on overflow set.
static int bvs_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    return branch(frame, loc.vaddr, frame->sr.vflow == 1);
}

const instruction_t INS_BCC = { "BCC", bcc_apply, false };
const instruction_t INS_BCS = { "BCS", bcs_apply, false };
const instruction_t INS_BEQ = { "BEQ", beq_apply, false };
const instruction_t INS_BMI = { "BMI", bmi_apply, false };
const instruction_t INS_BNE = { "BNE", bne_apply, false };
const instruction_t INS_BPL = { "BPL", bpl_apply, false };
const instruction_t INS_BVC = { "BVC", bvc_apply, false };
const instruction_t INS_BVS = { "BVS", bvs_apply, false };

/**
 * Jumps and subroutines.
 */

static int jmp_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->pc = loc.vaddr;
    return am == &AM_INDIRECT ? 5 : 3;
}

static int jrs_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    push_word(frame, as, frame->pc + 2);
    frame->pc = loc.vaddr;
    return 6;
}

static int rts_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->pc = pull_word(frame, as) + 1;
    return 6;
}

const instruction_t INS_JMP = { "JMP", jmp_apply, true };
const instruction_t INS_JSR = { "JSR", jrs_apply, true };
const instruction_t INS_RTS = { "RTS", rts_apply, true };

/**
 * Interrupts.
 */

static int brk_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    // Push PC and status register.
    push_word(frame, as, frame->pc + 2);
    push(frame, as, frame->sr.bits | SR_BREAK);

    // Disable interrupts.
    frame->sr.irq = 1;

    // Jump to interrupt handler.
    uint8_t low = as_read(as, IRQ_VECTOR);
    uint8_t high = as_read(as, IRQ_VECTOR + 1);
    frame->pc = bytes_to_word(low, high);
    
    // Process takes 7 cycles to complete.
    return 7;
}

static int rti_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t bits = pull(frame, as);
    uint8_t mask = SR_BREAK | SR_IGNORED;
    frame->sr.bits = (bits & ~mask) | (frame->sr.bits & mask);
    frame->pc = pull_word(frame, as);
    return 6;
}

const instruction_t INS_BRK = { "BRK", brk_apply, true };
const instruction_t INS_RTI = { "RTI", rti_apply, true };

/**
 * Other.
 */

static int bit_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    uint8_t result = frame->ac & value;
    frame->sr.neg = ((value & 0x80) > 0);
    frame->sr.vflow = ((value & 0x40) > 0);
    frame->sr.zero = (result == 0);
    return def_cycles(am, loc);
}

static int nop_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    return def_cycles(am, loc);
}

const instruction_t INS_BIT = { "BIT", bit_apply, false };
const instruction_t INS_NOP = { "NOP", nop_apply, false };

/**
 * Illegal instructions.
 */

static int alr_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    and_apply(frame, as, am, loc);
    lsr_apply(frame, as, am, loc);
    return def_cycles(am, loc);
}

static int anc_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    uint8_t result = frame->ac & value;
    frame->sr.carry = ((result & 0x80) == 0x80);
    update_sign_flags(frame, result);
    return def_cycles(am, loc);
}

static int dcp_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    dec_apply(frame, as, am, loc);
    cmp_apply(frame, as, am, loc);
    loc.page_boundary_crossed = true;
    return def_cycles(am, loc) + 2;
}

static int isc_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    inc_apply(frame, as, am, loc);
    sbc_apply(frame, as, am, loc);
    loc.page_boundary_crossed = true;
    return def_cycles(am, loc) + 2;
}

static int las_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    uint8_t value = load(as, loc);
    frame->ac = frame->sp & value;
    frame->x = frame->ac;
    frame->sp = frame->x;
    update_sign_flags(frame, frame->sp);
    return def_cycles(am, loc);
}

static int lax_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    frame->ac = load(as, loc);
    frame->x = frame->ac;
    update_sign_flags(frame, frame->x);
    return def_cycles(am, loc);
}

static int rla_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    int cycles = rol_apply(frame, as, am, loc);
    and_apply(frame, as, am, loc);
    return cycles;
}

static int rra_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    int cycles = ror_apply(frame, as, am, loc);
    adc_apply(frame, as, am, loc);
    return cycles;
}

static int sax_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    store(as, loc, frame->ac & frame->x);
    return def_cycles(am, loc);
}

static int sha_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    store(as, loc, frame->ac & frame->x & ((loc.vaddr + 1) >> 8));
    loc.page_boundary_crossed = true;
    return def_cycles(am, loc);
}

static int slo_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    int cycles = asl_apply(frame, as, am, loc);
    ora_apply(frame, as, am, loc);
    return cycles;
}

static int sre_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    int cycles = lsr_apply(frame, as, am, loc);
    eor_apply(frame, as, am, loc);
    return cycles;
}

static int usbc_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    return sbc_apply(frame, as, am, loc);
}

static int jam_apply(tframe_t *frame, const addrspace_t *as, const addrmode_t *am, mem_loc_t loc) {
    printf("JAM detected.\n");
    exit(1);
}

const instruction_t INS_ALR = { "ALR", alr_apply, false };
const instruction_t INS_ANC = { "ANC", anc_apply, false };
const instruction_t INS_ANE = { "ANE", NULL, false };
const instruction_t INS_ARR = { "ARR", NULL, false };
const instruction_t INS_DCP = { "DCP", dcp_apply, false };
const instruction_t INS_ISC = { "ISC", isc_apply, false };
const instruction_t INS_LAS = { "LAS", las_apply, false };
const instruction_t INS_LAX = { "LAX", lax_apply, false };
const instruction_t INS_LXA = { "LXA", NULL, false };
const instruction_t INS_RLA = { "RLA", rla_apply, false };
const instruction_t INS_RRA = { "RRA", rra_apply, false };
const instruction_t INS_SAX = { "SAX", sax_apply, false };
const instruction_t INS_SBX = { "SBX", NULL, false };
const instruction_t INS_SHA = { "SHA", sha_apply, false };
const instruction_t INS_SHX = { "SHX", NULL, false };
const instruction_t INS_SHY = { "SHY", NULL, false };
const instruction_t INS_SLO = { "SLO", slo_apply, false };
const instruction_t INS_SRE = { "SRE", sre_apply, false };
const instruction_t INS_TAS = { "TAS", NULL, false };
const instruction_t INS_USBC = { "USBC", usbc_apply, false };
const instruction_t INS_JAM = { "JAM", jam_apply, false };
