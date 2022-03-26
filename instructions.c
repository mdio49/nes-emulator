#include "instructions.h"

/**
 * Helper functions.
 */

static void update_sign_flags(tframe_t *frame, uint8_t result) {
    frame->sr.flag_zero = (result == 0x00);
    frame->sr.flag_negative = ((result & 0x80) == 0x80);
}

static void transfer(tframe_t *frame, uint8_t *dest, uint8_t value) {
    *dest = value;
    update_sign_flags(frame, value);
}

/**
 * Transfer instructions.
 */

static void lda_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    transfer(frame, &frame->ac, *value);
}

static void ldx_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    transfer(frame, &frame->x, *value);
}

static void ldy_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    transfer(frame, &frame->y, *value);
}

static void sta_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    *value = frame->ac;
}

static void stx_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    *value = frame->x;
}

static void sty_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    *value = frame->y;
}

static void tax_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    transfer(frame, &frame->x, frame->ac);
}

static void tay_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    transfer(frame, &frame->y, frame->ac);
}

static void tsx_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    transfer(frame, &frame->x, frame->sp);
}

static void txa_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    transfer(frame, &frame->ac, frame->x);
}

static void txs_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    transfer(frame, &frame->sp, frame->x);
}

static void tya_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    transfer(frame, &frame->ac, frame->y);
}

const instruction_t INS_LDA = { "LDA", lda_apply };
const instruction_t INS_LDX = { "LDX", ldx_apply };
const instruction_t INS_LDY = { "LDY", ldy_apply };

const instruction_t INS_STA = { "STA", sta_apply };
const instruction_t INS_STX = { "STX", stx_apply };
const instruction_t INS_STY = { "STY", sty_apply };

const instruction_t INS_TAX = { "TAX", tax_apply };
const instruction_t INS_TAY = { "TAY", tay_apply };
const instruction_t INS_TSX = { "TSX", tsx_apply };
const instruction_t INS_TXA = { "TXA", txa_apply };
const instruction_t INS_TXS = { "TXS", txs_apply };
const instruction_t INS_TYA = { "TYA", tya_apply };

/**
 * Stack instructions.
 */

const instruction_t INS_PHA = { "PHA", NULL };
const instruction_t INS_PHP = { "PHP", NULL };
const instruction_t INS_PLA = { "PLA", NULL };
const instruction_t INS_PLP = { "PLP", NULL };

/**
 * Decrements and increments.
 */

static void dec_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    update_sign_flags(frame, --(*value));
}

static void dex_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    update_sign_flags(frame, --frame->x);
}

static void dey_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    update_sign_flags(frame, --frame->y);
}

static void inc_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    update_sign_flags(frame, ++(*value));
}

static void inx_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    update_sign_flags(frame, ++frame->x);
}

static void iny_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    update_sign_flags(frame, ++frame->y);
}

const instruction_t INS_DEC = { "DEC", dec_apply };
const instruction_t INS_DEX = { "DEX", dex_apply };
const instruction_t INS_DEY = { "DEY", dey_apply };
const instruction_t INS_INC = { "INC", inc_apply };
const instruction_t INS_INX = { "INX", inx_apply };
const instruction_t INS_INY = { "INY", iny_apply };

/**
 * Arithmetic operators.
 */

const instruction_t INS_ADC = { "ADC", NULL };
const instruction_t INS_SBC = { "SBC", NULL };

/**
 * Logical operators.
 */

static void and_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->ac = frame->ac & *value;
    update_sign_flags(frame, frame->ac);
}

static void eor_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->ac = frame->ac ^ *value;
    update_sign_flags(frame, frame->ac);
}

static void ora_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->ac = frame->ac | *value;
    update_sign_flags(frame, frame->ac);
}

const instruction_t INS_AND = { "AND", and_apply };
const instruction_t INS_EOR = { "EOR", eor_apply };
const instruction_t INS_ORA = { "ORA", ora_apply };

/**
 * Shift & rotate instructions.
 */

static void asl_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    if ((*value & 0x80) == 0x80)
        frame->sr.flag_carry = 1;
    *value <<= 1;
    update_sign_flags(frame, *value);
}

static void lsr_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    if ((*value & 0x01) == 0x01)
        frame->sr.flag_carry = 1;
    *value >>= 1;
    update_sign_flags(frame, *value);
}

static void rol_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    if ((*value & 0x80) == 0x80)
        frame->sr.flag_carry = 1;
    *value = (*value << 1) | frame->sr.flag_carry;
    update_sign_flags(frame, *value);
}

static void ror_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    if ((*value & 0x01) == 0x01)
        frame->sr.flag_carry = 1;
    *value = (*value >> 1) | (frame->sr.flag_carry << 7);
    update_sign_flags(frame, *value);
}

const instruction_t INS_ASL = { "ASL", asl_apply };
const instruction_t INS_LSR = { "LSR", lsr_apply };
const instruction_t INS_ROL = { "ROL", rol_apply };
const instruction_t INS_ROR = { "ROR", ror_apply };

/**
 * Flag instructions.
 */

static void clc_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flag_carry = 0;
}

static void cld_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flag_decimal = 0;
}

static void cli_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flag_interrupt = 0;
}

static void clv_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flag_overflow = 0;
}

static void sec_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flag_carry = 1;
}

static void sed_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flag_decimal = 1;
}

static void sei_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flag_interrupt = 1;
}

const instruction_t INS_CLC = { "CLC", clc_apply };
const instruction_t INS_CLD = { "CLD", cld_apply };
const instruction_t INS_CLI = { "CLI", cli_apply };
const instruction_t INS_CLV = { "CLV", clv_apply };
const instruction_t INS_SEC = { "SEC", sec_apply };
const instruction_t INS_SED = { "SED", sed_apply };
const instruction_t INS_SEI = { "SEI", sei_apply };

/**
 * Comparisons.
 */

const instruction_t INS_CMP = { "CMP", NULL };
const instruction_t INS_CPX = { "CPX", NULL };
const instruction_t INS_CPY = { "CPY", NULL };

/**
 * Conditional branch instructions.
 */

const instruction_t INS_BCC = { "BCC", NULL };
const instruction_t INS_BCS = { "BCS", NULL };
const instruction_t INS_BEQ = { "BEQ", NULL };
const instruction_t INS_BMI = { "BMI", NULL };
const instruction_t INS_BNE = { "BNE", NULL };
const instruction_t INS_BPL = { "BPL", NULL };
const instruction_t INS_BVC = { "BVC", NULL };
const instruction_t INS_BVS = { "BVS", NULL };

/**
 * Jumps and subroutines.
 */

const instruction_t INS_JMP = { "JMP", NULL };
const instruction_t INS_JSR = { "JSR", NULL };
const instruction_t INS_RTS = { "RTS", NULL };

/**
 * Interrupts.
 */

static void brk_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flag_break = 1;
    // TODO
}

static void rti_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    // TODO
}

const instruction_t INS_BRK = { "BRK", brk_apply };
const instruction_t INS_RTI = { "RTI", rti_apply };

/**
 * Other.
 */

static void bit_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    // TODO
}

static void nop_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    // Do nothing.
}

const instruction_t INS_BIT = { "BIT", bit_apply };
const instruction_t INS_NOP = { "NOP", nop_apply };
