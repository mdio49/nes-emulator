#include "instructions.h"
#include <assert.h>
#include <stdio.h>

void test_instructions(tframe_t *frame, uint8_t *mem);

void test_load_store(tframe_t *frame, uint8_t *mem, uint8_t *reg, const instruction_t *load, const instruction_t *store);
void test_inc_dec(tframe_t *frame, uint8_t *mem, uint8_t *value, const instruction_t *inc, const instruction_t *dec);
void test_branch(tframe_t *frame, uint8_t *mem, const instruction_t *branch, uint8_t mask, unsigned int true_val);

int main() {
    // Setup CPU.
    tframe_t frame = { 0 };
    uint8_t memory[MEM_SIZE] = { 0 };
    frame.sr.flags.ign = 1;
    frame.sp = 0xFF;

    test_instructions(&frame, memory);

    printf("All tests passed successfully!\n");
}

void test_instructions(tframe_t *frame, uint8_t *mem) {
    uint8_t value;

    /**
     * Test load store.
     */

    test_load_store(frame, mem, &frame->ac, &INS_LDA, &INS_STA);
    test_load_store(frame, mem, &frame->x, &INS_LDX, &INS_STX);
    test_load_store(frame, mem, &frame->y, &INS_LDY, &INS_STY);

    /**
     * Test transfer.
     */

    frame->ac = 10;
    frame->x = 20;
    INS_TAX.apply(frame, mem, NULL);
    assert(frame->x == frame->ac);

    frame->ac = 10;
    frame->y = 20;
    INS_TAY.apply(frame, mem, NULL);
    assert(frame->y == frame->ac);

    frame->x = 0x2C;
    frame->sp = 0xFF;
    INS_TSX.apply(frame, mem, NULL);
    assert(frame->sp == frame->x);

    /**
     * Test stack.
     */

    uint8_t sp_start = frame->sp;

    frame->ac = 10;
    INS_PHA.apply(frame, mem, NULL);
    assert(frame->sp = sp_start - 1);
    frame->ac = 100;
    INS_PHA.apply(frame, mem, NULL);
    assert(frame->sp = sp_start - 2);
    frame->ac = 200;
    INS_PHA.apply(frame, mem, NULL);
    assert(frame->sp = sp_start - 3);

    INS_PLA.apply(frame, mem, NULL);
    assert(frame->ac == 200);
    assert(frame->sp = sp_start - 2);
    INS_PLA.apply(frame, mem, NULL);
    assert(frame->ac == 100);
    assert(frame->sp = sp_start - 1);
    INS_PLA.apply(frame, mem, NULL);
    assert(frame->ac == 10);
    assert(frame->sp = sp_start);

    srflags_t sr = frame->sr;
    INS_PHP.apply(frame, mem, NULL);
    frame->sr.bits = ~sr.bits;
    INS_PLP.apply(frame, mem, NULL);
    assert(frame->sr.flags.carry == sr.flags.carry);
    assert(frame->sr.flags.dec == sr.flags.dec);
    assert(frame->sr.flags.irq == sr.flags.irq);
    assert(frame->sr.flags.neg == sr.flags.neg);
    assert(frame->sr.flags.vflow == sr.flags.vflow);
    assert(frame->sr.flags.zero == sr.flags.zero);

    /**
     * Test decrements and increments.
     */

    test_inc_dec(frame, mem, &mem[0], &INS_INC, &INS_DEC);
    test_inc_dec(frame, mem, &frame->x, &INS_INX, &INS_DEX);
    test_inc_dec(frame, mem, &frame->y, &INS_INY, &INS_DEY);

    /**
     * Test arithmetic operators.
     */

    // ADC

    // Standard arithmetic.
    frame->sr.flags.dec = 0;

    frame->ac = 0x01;
    frame->sr.flags.carry = 0;
    value = 0x02;

    INS_ADC.apply(frame, mem, &value);
    assert(frame->ac == 0x03);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x01;
    frame->sr.flags.carry = 1;
    value = 0x02;

    INS_ADC.apply(frame, mem, &value);
    assert(frame->ac == 0x04);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x7F;
    frame->sr.flags.carry = 0;
    value = 0x01;

    INS_ADC.apply(frame, mem, &value);
    assert(frame->ac = 0x80);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0xFF;
    frame->sr.flags.carry = 0;
    value = 0x01;

    INS_ADC.apply(frame, mem, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 1);

    frame->ac = 0xFF;
    frame->sr.flags.carry = 0;
    value = 0x02;

    INS_ADC.apply(frame, mem, &value);
    assert(frame->ac == 0x01);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0xFF;
    frame->sr.flags.carry = 1;
    value = 0xFF;

    INS_ADC.apply(frame, mem, &value);
    assert(frame->ac == 0xFF);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    // Binary coded decimal.
    frame->sr.flags.dec = 1;

    frame->ac = 0x01;
    frame->sr.flags.carry = 0;
    value = 0x02;

    INS_ADC.apply(frame, mem, &value);
    assert(frame->ac == 0x03);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x01;
    frame->sr.flags.carry = 1;
    value = 0x02;

    INS_ADC.apply(frame, mem, &value);
    assert(frame->ac == 0x04);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x18;
    frame->sr.flags.carry = 0;
    value = 0x06;

    INS_ADC.apply(frame, mem, &value);
    assert(frame->ac == 0x24);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x79;
    frame->sr.flags.carry = 0;
    value = 0x01;

    INS_ADC.apply(frame, mem, &value);
    assert(frame->ac = 0x80);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x99;
    frame->sr.flags.carry = 0;
    value = 0x01;

    INS_ADC.apply(frame, mem, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 1);

    frame->ac = 0x99;
    frame->sr.flags.carry = 0;
    value = 0x02;

    INS_ADC.apply(frame, mem, &value);
    assert(frame->ac == 0x01);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    // SBC

    // TODO

    /**
     * Test logical operators.
     */

    // AND

    frame->ac = 0xF0;
    value = 0x0F;
    INS_AND.apply(frame, mem, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.zero == 1);
    assert(frame->sr.flags.neg == 0);

    frame->ac = 0x03;
    value = 0x06;
    INS_AND.apply(frame, mem, &value);
    assert(frame->ac == 0x02);
    assert(frame->sr.flags.zero == 0);
    assert(frame->sr.flags.neg == 0);

    frame->ac = 0xF0;
    value = 0xF0;
    INS_AND.apply(frame, mem, &value);
    assert(frame->ac == 0xF0);
    assert(frame->sr.flags.zero == 0);
    assert(frame->sr.flags.neg == 1);

    // OR

    frame->ac = 0xF0;
    value = 0x0F;
    INS_ORA.apply(frame, mem, &value);
    assert(frame->ac == 0xFF);
    assert(frame->sr.flags.zero == 0);
    assert(frame->sr.flags.neg == 1);

    frame->ac = 0x03;
    value = 0x06;
    INS_ORA.apply(frame, mem, &value);
    assert(frame->ac == 0x07);
    assert(frame->sr.flags.zero == 0);
    assert(frame->sr.flags.neg == 0);

    frame->ac = 0x00;
    value = 0x00;
    INS_ORA.apply(frame, mem, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.zero == 1);
    assert(frame->sr.flags.neg == 0);

    // XOR

    frame->ac = 0xF0;
    value = 0x0F;
    INS_EOR.apply(frame, mem, &value);
    assert(frame->ac == 0xFF);
    assert(frame->sr.flags.zero == 0);
    assert(frame->sr.flags.neg == 1);

    frame->ac = 0x03;
    value = 0x06;
    INS_EOR.apply(frame, mem, &value);
    assert(frame->ac == 0x05);
    assert(frame->sr.flags.zero == 0);
    assert(frame->sr.flags.neg == 0);

    frame->ac = 0xF0;
    value = 0xF0;
    INS_EOR.apply(frame, mem, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.zero == 1);
    assert(frame->sr.flags.neg == 0);

    /**
     * Test shift and rotate instructions.
     */

    // ASL (left shift).

    frame->ac = 0x20;
    frame->sr.flags.carry = 1;
    frame->sr.flags.neg = 1;
    frame->sr.flags.zero = 1;

    INS_ASL.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x40);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    INS_ASL.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x80);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    INS_ASL.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    INS_ASL.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    // LSR (right shift).

    frame->ac = 0x02;
    frame->sr.flags.carry = 1;
    frame->sr.flags.neg = 1;
    frame->sr.flags.zero = 1;

    INS_LSR.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x01);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    INS_LSR.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    INS_LSR.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    frame->ac = 0x90;
    frame->sr.flags.carry = 1;
    frame->sr.flags.neg = 1;
    frame->sr.flags.zero = 1;

    INS_LSR.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x48);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    // ROL (rotate left).

    frame->ac = 0x41;
    frame->sr.flags.carry = 0;
    frame->sr.flags.neg = 0;
    frame->sr.flags.zero = 0;

    INS_ROL.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x82);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    INS_ROL.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x05);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x00;
    INS_ROL.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    // ROR (rotate right).

    frame->ac = 0x05;
    frame->sr.flags.carry = 0;
    frame->sr.flags.neg = 0;
    frame->sr.flags.zero = 0;

    INS_ROR.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x82);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    INS_ROR.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x41);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x00;
    INS_ROR.apply(frame, mem, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    /**
     * Test flag instructions (set and clear).
     */

    frame->sr.flags.carry = 0;
    frame->sr.flags.dec = 0;
    frame->sr.flags.irq = 0;
    frame->sr.flags.vflow = 1;

    INS_SEC.apply(frame, mem, NULL);
    assert(frame->sr.flags.carry == 1);
    INS_SED.apply(frame, mem, NULL);
    assert(frame->sr.flags.dec == 1);
    INS_SEI.apply(frame, mem, NULL);
    assert(frame->sr.flags.irq == 1);

    INS_CLC.apply(frame, mem, NULL);
    assert(frame->sr.flags.carry == 0);
    INS_CLD.apply(frame, mem, NULL);
    assert(frame->sr.flags.dec == 0);
    INS_CLI.apply(frame, mem, NULL);
    assert(frame->sr.flags.irq == 0);
    INS_CLV.apply(frame, mem, NULL);
    assert(frame->sr.flags.vflow == 0);

    /**
     * Test comparisons.
     */

    // TODO

    /**
     * Test conditional branch instructions.
     */

    test_branch(frame, mem, &INS_BCC, SR_CARRY, 0);
    test_branch(frame, mem, &INS_BCS, SR_CARRY, 1);
    test_branch(frame, mem, &INS_BEQ, SR_ZERO, 1);
    test_branch(frame, mem, &INS_BMI, SR_NEGATIVE, 1);
    test_branch(frame, mem, &INS_BNE, SR_ZERO, 0);
    test_branch(frame, mem, &INS_BPL, SR_NEGATIVE, 0);
    test_branch(frame, mem, &INS_BVC, SR_OVERFLOW, 0);
    test_branch(frame, mem, &INS_BVS, SR_OVERFLOW, 1);

    /**
     * Test jumps and subroutines.
     */

    const uint16_t pc_start = AS_PROG;
    const uint16_t pc_target = pc_start + 300;

    frame->pc = pc_start + 3;
    INS_JMP.apply(frame, mem, mem + pc_target);
    assert(frame->pc == pc_target);

    frame->sp = 255;
    frame->pc = pc_start + 3;
    INS_JSR.apply(frame, mem, mem + pc_target);
    assert(frame->pc == pc_target);
    assert(frame->sp == 253);

    frame->pc += 100;
    INS_RTS.apply(frame, mem, NULL);
    assert(frame->pc == pc_start + 3);
    assert(frame->sp == 255);

    /**
     * Test interrupts.
     */

    frame->sr.flags.brk = 0;
    INS_BRK.apply(frame, mem, NULL);
    assert(frame->sr.flags.brk == 1);

    /**
     * Test no-op.
     */

    tframe_t prev = *frame;
    uint8_t oldmem[MEM_SIZE];
    for (int i = 0; i < MEM_SIZE; i++) {
        oldmem[i] = mem[i];
    }

    INS_NOP.apply(frame, mem, NULL);
    assert(prev.ac == frame->ac);
    assert(prev.pc == frame->pc);
    assert(prev.sp == frame->sp);
    assert(prev.sr.bits == frame->sr.bits);
    assert(prev.sp == frame->sp);
    assert(prev.x == frame->x);
    assert(prev.y == frame->y);
    for (int i = 0; i < MEM_SIZE; i++) {
        assert(oldmem[i] == mem[i]);
    }
}

void test_load_store(tframe_t *frame, uint8_t *mem, uint8_t *reg, const instruction_t *load, const instruction_t *store) {
    uint8_t value;

    value = 0x00;
    load->apply(frame, mem, &value);
    assert(*reg == value);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    value = 0x05;
    load->apply(frame, mem, &value);
    assert(*reg == value);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    store->apply(frame, mem, &mem[0x20]);
    assert(mem[0x20] == value);

    value = 0xF9;
    load->apply(frame, mem, &value);
    assert(*reg == value);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    store->apply(frame, mem, &mem[0x21]);
    assert(mem[0x21] == value);
}

void test_inc_dec(tframe_t *frame, uint8_t *mem, uint8_t *value, const instruction_t *inc, const instruction_t *dec) {
    *value = 2;

    dec->apply(frame, mem, value);
    assert(*value == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    dec->apply(frame, mem, value);
    assert(*value == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    dec->apply(frame, mem, value);
    assert(*value == 255);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    dec->apply(frame, mem, value);
    assert(*value == 254);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    inc->apply(frame, mem, value);
    assert(*value == 255);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    inc->apply(frame, mem, value);
    assert(*value == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    inc->apply(frame, mem, value);
    assert(*value == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    *value = 129;

    dec->apply(frame, mem, value);
    assert(*value == 128);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    dec->apply(frame, mem, value);
    assert(*value == 127);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    dec->apply(frame, mem, value);
    assert(*value == 126);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);
    
    inc->apply(frame, mem, value);
    assert(*value == 127);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    inc->apply(frame, mem, value);
    assert(*value == 128);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    inc->apply(frame, mem, value);
    assert(*value == 129);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);
}

void test_branch(tframe_t *frame, uint8_t *mem, const instruction_t *branch, uint8_t mask, unsigned int true_val) {
    uint8_t value = 5;
    frame->pc = AS_PROG;

    frame->sr.bits = !true_val ? mask : 0x00;
    branch->apply(frame, mem, &value);
    assert(frame->pc == AS_PROG);

    frame->sr.bits = true_val ? mask : 0x00;
    branch->apply(frame, mem, &value);
    assert(frame->pc == AS_PROG + 5);

    frame->pc = AS_PROG;
    value = -5;

    frame->sr.bits = true_val ? mask : 0x00;
    branch->apply(frame, mem, &value);
    assert(frame->pc == AS_PROG - 5);
}
