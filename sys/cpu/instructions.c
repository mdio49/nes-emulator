#include "instructions.h"
#include <stdbool.h>

/**
 * Helper functions.
 */

static void update_sign_flags(tframe_t *frame, uint8_t result) {
    frame->sr.flags.zero = (result == 0x00);
    frame->sr.flags.neg = ((result & 0x80) == 0x80);
}

static void transfer(tframe_t *frame, uint8_t *dest, uint8_t value) {
    *dest = value;
    update_sign_flags(frame, value);
}

static void push(tframe_t *frame, uint8_t *mem, uint8_t value) {
    mem[STACK_START + frame->sp] = value;
    frame->sp--;
}

static uint8_t pull(tframe_t *frame, uint8_t *mem) {
    frame->sp++;
    uint8_t value = mem[STACK_START + frame->sp];
    update_sign_flags(frame, value);
    return value;
}

static void push_word(tframe_t *frame, uint8_t *mem, uint16_t value) {
    push(frame, mem, value >> 8);       // high
    push(frame, mem, value & 0x00FF);   // low
}

static uint16_t pull_word(tframe_t *frame, uint8_t *mem) {
    uint8_t low = pull(frame, mem);
    uint8_t high = pull(frame, mem);
    return word(low, high);
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

static void pha_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    push(frame, mem, frame->ac);
}

static void php_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    push(frame, mem, frame->sr.bits);
}

static void pla_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->ac = pull(frame, mem);
}

static void plp_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.bits = pull(frame, mem);
}

const instruction_t INS_PHA = { "PHA", pha_apply };
const instruction_t INS_PHP = { "PHP", php_apply };
const instruction_t INS_PLA = { "PLA", pla_apply };
const instruction_t INS_PLP = { "PLP", plp_apply };

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

static uint8_t from_bcd(uint8_t bin) {
    return (bin >> 4) * 10 + (bin & 0x0F);
}

static uint8_t to_bcd(uint8_t value) {
    return ((value / 10) << 4) + (value % 10);
}

static void adc_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    uint8_t carry = frame->sr.flags.carry;
    if (frame->sr.flags.dec == 0) {
        // Standard binary arithmetic.
        frame->sr.flags.carry = ((int16_t)frame->ac + *value + carry > 255);
        frame->ac = frame->ac + *value + carry;
    }
    else {
        // Binary coded decimal.
        uint8_t a = from_bcd(frame->ac);
        uint8_t m = from_bcd(*value);
        int16_t result = a + m + carry;

        frame->sr.flags.carry = result >= 100;
        frame->ac = to_bcd(result % 100);
    }
    
    update_sign_flags(frame, frame->ac);
}

static void sdc_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    uint8_t carry = frame->sr.flags.carry ^ 0x01;
    if (frame->sr.flags.dec == 0) {
        // Standard binary arithmetic.
        frame->sr.flags.carry = ((int16_t)frame->ac - *value - carry >= 0);
        frame->ac = frame->ac - *value - carry;
    }
    else {
        // Binary coded decimal.
        uint8_t a = from_bcd(frame->ac);
        uint8_t m = from_bcd(*value);
        int16_t result = a - m - carry;

        frame->sr.flags.carry = result >= 0;
        frame->ac = to_bcd((100 + (result % 100)) % 100);
    }

    update_sign_flags(frame, frame->ac);
}

const instruction_t INS_ADC = { "ADC", adc_apply };
const instruction_t INS_SBC = { "SBC", sdc_apply };

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
    frame->sr.flags.carry = ((*value & 0x80) == 0x80);
    *value <<= 1;
    update_sign_flags(frame, *value);
}

static void lsr_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flags.carry = ((*value & 0x01) == 0x01);
    *value >>= 1;
    update_sign_flags(frame, *value);
}

static void rol_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flags.carry = ((*value & 0x80) == 0x80);
    *value = (*value << 1) | frame->sr.flags.carry;
    update_sign_flags(frame, *value);
}

static void ror_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flags.carry = ((*value & 0x01) == 0x01);
    *value = (*value >> 1) | (frame->sr.flags.carry << 7);
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
    frame->sr.flags.carry = 0;
}

static void cld_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flags.dec = 0;
}

static void cli_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flags.irq = 0;
}

static void clv_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flags.vflow = 0;
}

static void sec_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flags.carry = 1;
}

static void sed_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flags.dec = 1;
}

static void sei_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flags.irq = 1;
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

static void compare(tframe_t *frame, uint8_t reg, uint8_t value) {
    uint8_t result = reg - value;
    frame->sr.flags.carry = (reg >= value);
    update_sign_flags(frame, result);
}

static void cmp_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    compare(frame, frame->ac, *value);
}

static void cmx_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    compare(frame, frame->x, *value);
}

static void cmy_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    compare(frame, frame->y, *value);
}

const instruction_t INS_CMP = { "CMP", cmp_apply };
const instruction_t INS_CPX = { "CPX", cmx_apply };
const instruction_t INS_CPY = { "CPY", cmy_apply };

/**
 * Conditional branch instructions.
 */

static void branch(tframe_t *frame, int8_t offset, bool condition) {
    if (condition) {
        frame->pc += offset;
    }
}

// Branch on carry clear.
static void bcc_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    branch(frame, *value, frame->sr.flags.carry == 0);
}

// Branch on carry set.
static void bcs_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    branch(frame, *value, frame->sr.flags.carry == 1);
}

// Branch on result zero.
static void beq_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    branch(frame, *value, frame->sr.flags.zero == 1);
}

// Branch on result minus (negative).
static void bmi_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    branch(frame, *value, frame->sr.flags.neg == 1);
}

// Branch on result not zero.
static void bne_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    branch(frame, *value, frame->sr.flags.zero == 0);
}

// Branch on result plus (positive).
static void bpl_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    branch(frame, *value, frame->sr.flags.neg == 0);
}

// Branch on overflow clear.
static void bvc_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    branch(frame, *value, frame->sr.flags.vflow == 0);
}

// Branch on overflow set.
static void bvs_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    branch(frame, *value, frame->sr.flags.vflow == 1);
}

const instruction_t INS_BCC = { "BCC", bcc_apply };
const instruction_t INS_BCS = { "BCS", bcs_apply };
const instruction_t INS_BEQ = { "BEQ", beq_apply };
const instruction_t INS_BMI = { "BMI", bmi_apply };
const instruction_t INS_BNE = { "BNE", bne_apply };
const instruction_t INS_BPL = { "BPL", bpl_apply };
const instruction_t INS_BVC = { "BVC", bvc_apply };
const instruction_t INS_BVS = { "BVS", bvs_apply };

/**
 * Jumps and subroutines.
 */

static void jmp_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->pc = (uint16_t)(value - mem); // The value points to the instruction we want to jump to.
}

static void jrs_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    push_word(frame, mem, frame->pc - 1);
    jmp_apply(frame, mem, value); 
}

static void rts_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->pc = pull_word(frame, mem) + 1;
}

const instruction_t INS_JMP = { "JMP", jmp_apply };
const instruction_t INS_JSR = { "JSR", jrs_apply };
const instruction_t INS_RTS = { "RTS", rts_apply };

/**
 * Interrupts.
 */

static void brk_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.flags.brk = 1;
    push_word(frame, mem, frame->pc + 1);
    push(frame, mem, frame->sr.bits);
}

static void rti_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    frame->sr.bits = pull(frame, mem);
    frame->pc = pull_word(frame, mem);
}

const instruction_t INS_BRK = { "BRK", brk_apply };
const instruction_t INS_RTI = { "RTI", rti_apply };

/**
 * Other.
 */

static void bit_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    uint8_t result = frame->ac & *value;
    frame->sr.flags.neg = ((*value & 0x80) == 0x80);
    frame->sr.flags.vflow = ((*value & 0x40) == 0x40);
    frame->sr.flags.zero = (result == 0);
}

static void nop_apply(tframe_t *frame, uint8_t *mem, uint8_t *value) {
    // Do nothing.
}

const instruction_t INS_BIT = { "BIT", bit_apply };
const instruction_t INS_NOP = { "NOP", nop_apply };
