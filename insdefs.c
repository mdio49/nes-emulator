#include "cpu.h"

/**
 * Transfer instructions.
 */

static void lda_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    frame->ac = mem[addr];
}

static void lda_apply_immediate(tframe_t *frame, uint8_t *mem, uint8_t value) {
    frame->ac = value;
}

static void ldx_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    frame->x = mem[addr];
}

static void ldx_apply_immediate(tframe_t *frame, uint8_t *mem, uint8_t value) {
    frame->x = value;
}

static void ldy_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    frame->y = mem[addr];
}

static void ldy_apply_immediate(tframe_t *frame, uint8_t *mem, uint8_t value) {
    frame->y = value;
}

static void sta_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    mem[addr] = frame->ac;
}

static void stx_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    mem[addr] = frame->x;
}

static void sty_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    mem[addr] = frame->y;
}

static void tax_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    frame->x = frame->ac;
}

static void tay_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    frame->y = frame->ac;
}

static void tsx_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    frame->x = frame->sp;
}

static void txa_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    frame->ac = frame->x;
}

static void txs_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    frame->sp = frame->x;
}

static void tya_apply(tframe_t *frame, uint8_t *mem, addr_t addr) {
    frame->ac  = frame->y;
}

const instruction_t INS_LDA = { "LDA", lda_apply, lda_apply_immediate };
const instruction_t INS_LDX = { "LDX", ldx_apply, ldx_apply_immediate };
const instruction_t INS_LDY = { "LDY", ldy_apply, ldy_apply_immediate };

const instruction_t INS_STA = { "STA", sta_apply, NULL };
const instruction_t INS_STX = { "STX", stx_apply, NULL };
const instruction_t INS_STY = { "STY", sty_apply, NULL };

const instruction_t INS_TAX = { "TAX", tax_apply, NULL };
const instruction_t INS_TAY = { "TAY", tay_apply, NULL };
const instruction_t INS_TSX = { "TSX", tsx_apply, NULL };
const instruction_t INS_TXA = { "TXA", txa_apply, NULL };
const instruction_t INS_TXS = { "TXS", txs_apply, NULL };
const instruction_t INS_TYA = { "TYA", tya_apply, NULL };

/**
 * Stack instructions.
 */

const instruction_t INS_PHA = { "PHA", NULL, NULL };
const instruction_t INS_PHP = { "PHP", NULL, NULL };
const instruction_t INS_PLA = { "PLA", NULL, NULL };
const instruction_t INS_PLP = { "PLP", NULL, NULL };

/**
 * Decrements and increments.
 */

const instruction_t INS_DEC = { "DEC", NULL, NULL };
const instruction_t INS_DEX = { "DEX", NULL, NULL };
const instruction_t INS_DEY = { "DEY", NULL, NULL };
const instruction_t INS_INC = { "INC", NULL, NULL };
const instruction_t INS_INX = { "INX", NULL, NULL };
const instruction_t INS_INY = { "INY", NULL, NULL };

/**
 * Arithmetic operators.
 */

const instruction_t INS_ADC = { "ADC", NULL, NULL };
const instruction_t INS_SBC = { "SBC", NULL, NULL };

/**
 * Logical operators.
 */

const instruction_t INS_AND = { "AND", NULL, NULL };
const instruction_t INS_EOR = { "EOR", NULL, NULL };
const instruction_t INS_ORA = { "ORA", NULL, NULL };

/**
 * Shift & rotate instructions.
 */

const instruction_t INS_ASL = { "ASL", NULL, NULL };
const instruction_t INS_LSR = { "LSR", NULL, NULL };
const instruction_t INS_ROL = { "ROL", NULL, NULL };
const instruction_t INS_ROR = { "ROR", NULL, NULL };

/**
 * Flag instructions.
 */

const instruction_t INS_CLC = { "CLC", NULL, NULL };
const instruction_t INS_CLD = { "CLD", NULL, NULL };
const instruction_t INS_CLI = { "CLI", NULL, NULL };
const instruction_t INS_CLV = { "CLV", NULL, NULL };
const instruction_t INS_SEC = { "SEC", NULL, NULL };
const instruction_t INS_SED = { "SED", NULL, NULL };
const instruction_t INS_SEI = { "SEI", NULL, NULL };

/**
 * Comparisons.
 */

const instruction_t INS_CMP = { "CMP", NULL, NULL };
const instruction_t INS_CPX = { "CPX", NULL, NULL };
const instruction_t INS_CPY = { "CPY", NULL, NULL };

/**
 * Conditional branch instructions.
 */

const instruction_t INS_BCC = { "BCC", NULL, NULL };
const instruction_t INS_BCS = { "BCS", NULL, NULL };
const instruction_t INS_BEQ = { "BEQ", NULL, NULL };
const instruction_t INS_BMI = { "BMI", NULL, NULL };
const instruction_t INS_BNE = { "BNE", NULL, NULL };
const instruction_t INS_BPL = { "BPL", NULL, NULL };
const instruction_t INS_BVC = { "BVC", NULL, NULL };
const instruction_t INS_BVS = { "BVS", NULL, NULL };

/**
 * Jumps and subroutines.
 */

const instruction_t INS_JMP = { "JMP", NULL, NULL };
const instruction_t INS_JSR = { "JSR", NULL, NULL };
const instruction_t INS_RTS = { "RTS", NULL, NULL };

/**
 * Interrupts.
 */

const instruction_t INS_BRK = { "BRK", NULL, NULL };
const instruction_t INS_RTI = { "RTI", NULL, NULL };

/**
 * Other.
 */

const instruction_t INS_BIT = { "BIT", NULL, NULL };
const instruction_t INS_NOP = { "NOP", NULL, NULL };
