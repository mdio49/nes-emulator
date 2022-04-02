#include <assert.h>
#include <addrmodes.h>
#include <instructions.h>
#include <stdio.h>

void test_address_modes(tframe_t *frame);
void test_instructions(tframe_t *frame);

void test_load_store(tframe_t *frame, const addrspace_t *as, uint8_t *reg, const instruction_t *load, const instruction_t *store);
void test_inc_dec(tframe_t *frame, const addrspace_t *as, uint8_t *value, const instruction_t *inc, const instruction_t *dec);
void test_compare(tframe_t *frame, const addrspace_t *as, uint8_t *reg, const instruction_t *cmp);
void test_branch(tframe_t *frame, const addrspace_t *as, const instruction_t *branch, uint8_t mask, unsigned int true_val);

int main() {
    tframe_t frame;
    test_address_modes(&frame);
    test_instructions(&frame);
    printf("All tests passed successfully!\n");
}

void test_address_modes(tframe_t *frame) {
    uint8_t args[3];
    uint8_t mem[1024];

    // Set up a very simple 1KB address space.
    addrspace_t *as = as_create();
    as_add_segment(as, 0, 1024, mem);

    // Immediate.
    args[0] = 0xFF;
    assert(*AM_IMMEDIATE.evaluate(frame, as, args).ptr == 0xFF);

    // Accumulator.
    frame->ac = 0x04;
    assert(*AM_ACCUMULATOR.evaluate(frame, as, args).ptr == 0x04);

    // Zero page.
    args[0] = 0x08;
    mem[0x08] = 0x80;
    assert(*AM_ZEROPAGE.evaluate(frame, as, args).ptr == 0x80);
    assert(AM_ZEROPAGE.evaluate(frame, as, args).vaddr == 0x08);

    // Zero page X.
    args[0] = 0x08;
    frame->x = 0x10;
    frame->y = 0x00;
    mem[0x18] = 0x40;
    assert(*AM_ZEROPAGE_X.evaluate(frame, as, args).ptr == 0x40);
    assert(AM_ZEROPAGE_X.evaluate(frame, as, args).vaddr == 0x18);

    args[0] = 0xFF;
    frame->x = 0x01;
    frame->y = 0x00;
    mem[0x00] = 0x02;
    assert(*AM_ZEROPAGE_X.evaluate(frame, as, args).ptr == 0x02);
    assert(AM_ZEROPAGE_X.evaluate(frame, as, args).vaddr == 0x00);

    // Zero page Y.
    args[0] = 0x08;
    frame->x = 0x00;
    frame->y = 0x10;
    mem[0x18] = 0x20;
    assert(*AM_ZEROPAGE_Y.evaluate(frame, as, args).ptr == 0x20);
    assert(AM_ZEROPAGE_Y.evaluate(frame, as, args).vaddr == 0x18);

    args[0] = 0xFF;
    frame->x = 0x00;
    frame->y = 0x01;
    mem[0x00] = 0x04;
    assert(*AM_ZEROPAGE_Y.evaluate(frame, as, args).ptr == 0x04);
    assert(AM_ZEROPAGE_Y.evaluate(frame, as, args).vaddr == 0x00);

    // Absolute.
    args[0] = 0xFF;
    args[1] = 0x01;
    mem[0x01FF] = 0x05;
    assert(*AM_ABSOLUTE.evaluate(frame, as, args).ptr == 0x05);
    assert(AM_ABSOLUTE.evaluate(frame, as, args).vaddr == 0x01FF);

    // Indirect.
    args[0] = 0x10;
    args[1] = 0x02;
    mem[0x0210] = 0x0C;
    mem[0x0211] = 0x01;
    mem[0x010C] = 0xAF;
    assert(*AM_INDIRECT.evaluate(frame, as, args).ptr == 0xAF);
    assert(AM_INDIRECT.evaluate(frame, as, args).vaddr == 0x010C);

    as_destroy(as);
}

void test_instructions(tframe_t *frame) {
    uint8_t value;
    uint8_t mem[65536];

    // Set up a simple address space that uses a single 64KB segment.
    addrspace_t *as = as_create();
    as_add_segment(as, 0, 65536, mem);

    /**
     * Test load store.
     */

    test_load_store(frame, as, &frame->ac, &INS_LDA, &INS_STA);
    test_load_store(frame, as, &frame->x, &INS_LDX, &INS_STX);
    test_load_store(frame, as, &frame->y, &INS_LDY, &INS_STY);

    /**
     * Test transfer.
     */

    frame->ac = 10;
    frame->x = 20;
    INS_TAX.apply(frame, as, 0, NULL);
    assert(frame->x == frame->ac);

    frame->ac = 10;
    frame->y = 20;
    INS_TAY.apply(frame, as, 0, NULL);
    assert(frame->y == frame->ac);

    frame->x = 0x2C;
    frame->sp = 0xFF;
    INS_TSX.apply(frame, as, 0, NULL);
    assert(frame->sp == frame->x);

    /**
     * Test stack.
     */

    uint8_t sp_start = frame->sp;

    frame->ac = 10;
    INS_PHA.apply(frame, as, 0, NULL);
    assert(frame->sp = sp_start - 1);
    frame->ac = 100;
    INS_PHA.apply(frame, as, 0, NULL);
    assert(frame->sp = sp_start - 2);
    frame->ac = 200;
    INS_PHA.apply(frame, as, 0, NULL);
    assert(frame->sp = sp_start - 3);

    INS_PLA.apply(frame, as, 0, NULL);
    assert(frame->ac == 200);
    assert(frame->sp = sp_start - 2);
    INS_PLA.apply(frame, as, 0, NULL);
    assert(frame->ac == 100);
    assert(frame->sp = sp_start - 1);
    INS_PLA.apply(frame, as, 0, NULL);
    assert(frame->ac == 10);
    assert(frame->sp = sp_start);

    srflags_t sr = frame->sr;
    INS_PHP.apply(frame, as, 0, NULL);
    frame->sr.bits = ~sr.bits;
    INS_PLP.apply(frame, as, 0, NULL);
    assert(frame->sr.flags.carry == sr.flags.carry);
    assert(frame->sr.flags.dec == sr.flags.dec);
    assert(frame->sr.flags.irq == sr.flags.irq);
    assert(frame->sr.flags.neg == sr.flags.neg);
    assert(frame->sr.flags.vflow == sr.flags.vflow);
    assert(frame->sr.flags.zero == sr.flags.zero);

    /**
     * Test decrements and increments.
     */

    test_inc_dec(frame, as, &value, &INS_INC, &INS_DEC);
    test_inc_dec(frame, as, &frame->x, &INS_INX, &INS_DEX);
    test_inc_dec(frame, as, &frame->y, &INS_INY, &INS_DEY);

    /**
     * Test arithmetic operators.
     */

    // ADC

    // Standard arithmetic.
    frame->sr.flags.dec = 0;

    frame->ac = 0x01;
    frame->sr.flags.carry = 0;
    value = 0x02;

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x03);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x01;
    frame->sr.flags.carry = 1;
    value = 0x02;

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x04);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x7F;
    frame->sr.flags.carry = 0;
    value = 0x01;

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac = 0x80);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0xFF;
    frame->sr.flags.carry = 0;
    value = 0x01;

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 1);

    frame->ac = 0xFF;
    frame->sr.flags.carry = 0;
    value = 0x02;

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x01);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0xFF;
    frame->sr.flags.carry = 1;
    value = 0xFF;

    INS_ADC.apply(frame, as, 0, &value);
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

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x03);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x01;
    frame->sr.flags.carry = 1;
    value = 0x02;

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x04);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x18;
    frame->sr.flags.carry = 0;
    value = 0x06;

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x24);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x79;
    frame->sr.flags.carry = 0;
    value = 0x01;

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac = 0x80);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x99;
    frame->sr.flags.carry = 0;
    value = 0x01;

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 1);

    frame->ac = 0x99;
    frame->sr.flags.carry = 0;
    value = 0x02;

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x01);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x99;
    frame->sr.flags.carry = 1;
    value = 0x99;

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x99);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    // BCD overflow cases.
    frame->ac = 0x0A;               // 10
    frame->sr.flags.carry = 0;
    value = 0x00;                   // +0

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x10);      // 10
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0xA0;               // 100
    frame->sr.flags.carry = 0;
    value = 0x00;                   // + 0

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x00);      // 100
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 1);

    frame->ac = 0xFF;               // 165
    frame->sr.flags.carry = 0;
    value = 0x00;                   // + 0

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x65);      // 165
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);
    
    frame->ac = 0xFF;               // 165
    frame->sr.flags.carry = 0;
    value = 0x34;                   // +34

    INS_ADC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x99);      // 199
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    // SBC

    // Standard arithmetic.
    frame->sr.flags.dec = 0;

    frame->ac = 0x08;
    frame->sr.flags.carry = 1;
    value = 0x02;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x06);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x08;
    frame->sr.flags.carry = 1;
    value = 0x08;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 1);

    frame->ac = 0x08;
    frame->sr.flags.carry = 1;
    value = 0x09;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0xFF);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x08;
    frame->sr.flags.carry = 0;
    value = 0x02;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x05);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x81;
    frame->sr.flags.carry = 1;
    value = 0x02;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x7F);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x85;
    frame->sr.flags.carry = 1;
    value = 0x02;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x83);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x00;
    frame->sr.flags.carry = 1;
    value = 0xFF;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x01);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    // Binary coded decimal.
    frame->sr.flags.dec = 1;

    frame->ac = 0x08;
    frame->sr.flags.carry = 1;
    value = 0x02;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x06);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x08;
    frame->sr.flags.carry = 1;
    value = 0x08;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 1);

    frame->ac = 0x08;
    frame->sr.flags.carry = 1;
    value = 0x09;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x99);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x08;
    frame->sr.flags.carry = 0;
    value = 0x02;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x05);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x81;
    frame->sr.flags.carry = 1;
    value = 0x02;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x79);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x85;
    frame->sr.flags.carry = 1;
    value = 0x02;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x83);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x00;
    frame->sr.flags.carry = 1;
    value = 0x99;

    INS_SBC.apply(frame, as, 0, &value);
    assert(frame->ac == 0x01);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.vflow == 0);
    assert(frame->sr.flags.zero == 0);

    /**
     * Test logical operators.
     */

    // AND

    frame->ac = 0xF0;
    value = 0x0F;
    INS_AND.apply(frame, as, 0, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.zero == 1);
    assert(frame->sr.flags.neg == 0);

    frame->ac = 0x03;
    value = 0x06;
    INS_AND.apply(frame, as, 0, &value);
    assert(frame->ac == 0x02);
    assert(frame->sr.flags.zero == 0);
    assert(frame->sr.flags.neg == 0);

    frame->ac = 0xF0;
    value = 0xF0;
    INS_AND.apply(frame, as, 0, &value);
    assert(frame->ac == 0xF0);
    assert(frame->sr.flags.zero == 0);
    assert(frame->sr.flags.neg == 1);

    // OR

    frame->ac = 0xF0;
    value = 0x0F;
    INS_ORA.apply(frame, as, 0, &value);
    assert(frame->ac == 0xFF);
    assert(frame->sr.flags.zero == 0);
    assert(frame->sr.flags.neg == 1);

    frame->ac = 0x03;
    value = 0x06;
    INS_ORA.apply(frame, as, 0, &value);
    assert(frame->ac == 0x07);
    assert(frame->sr.flags.zero == 0);
    assert(frame->sr.flags.neg == 0);

    frame->ac = 0x00;
    value = 0x00;
    INS_ORA.apply(frame, as, 0, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.zero == 1);
    assert(frame->sr.flags.neg == 0);

    // XOR

    frame->ac = 0xF0;
    value = 0x0F;
    INS_EOR.apply(frame, as, 0, &value);
    assert(frame->ac == 0xFF);
    assert(frame->sr.flags.zero == 0);
    assert(frame->sr.flags.neg == 1);

    frame->ac = 0x03;
    value = 0x06;
    INS_EOR.apply(frame, as, 0, &value);
    assert(frame->ac == 0x05);
    assert(frame->sr.flags.zero == 0);
    assert(frame->sr.flags.neg == 0);

    frame->ac = 0xF0;
    value = 0xF0;
    INS_EOR.apply(frame, as, 0, &value);
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

    INS_ASL.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x40);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    INS_ASL.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x80);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    INS_ASL.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    INS_ASL.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    // LSR (right shift).

    frame->ac = 0x02;
    frame->sr.flags.carry = 1;
    frame->sr.flags.neg = 1;
    frame->sr.flags.zero = 1;

    INS_LSR.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x01);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    INS_LSR.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    INS_LSR.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    frame->ac = 0x90;
    frame->sr.flags.carry = 1;
    frame->sr.flags.neg = 1;
    frame->sr.flags.zero = 1;

    INS_LSR.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x48);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    // ROL (rotate left).

    frame->ac = 0x41;
    frame->sr.flags.carry = 0;
    frame->sr.flags.neg = 0;
    frame->sr.flags.zero = 0;

    INS_ROL.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x82);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    INS_ROL.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x05);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x00;
    INS_ROL.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    // ROR (rotate right).

    frame->ac = 0x05;
    frame->sr.flags.carry = 0;
    frame->sr.flags.neg = 0;
    frame->sr.flags.zero = 0;

    INS_ROR.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x82);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    INS_ROR.apply(frame, as, 0, &frame->ac);
    assert(frame->ac == 0x41);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    frame->ac = 0x00;
    INS_ROR.apply(frame, as, 0, &frame->ac);
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

    INS_SEC.apply(frame, as, 0, NULL);
    assert(frame->sr.flags.carry == 1);
    INS_SED.apply(frame, as, 0, NULL);
    assert(frame->sr.flags.dec == 1);
    INS_SEI.apply(frame, as, 0, NULL);
    assert(frame->sr.flags.irq == 1);

    INS_CLC.apply(frame, as, 0, NULL);
    assert(frame->sr.flags.carry == 0);
    INS_CLD.apply(frame, as, 0, NULL);
    assert(frame->sr.flags.dec == 0);
    INS_CLI.apply(frame, as, 0, NULL);
    assert(frame->sr.flags.irq == 0);
    INS_CLV.apply(frame, as, 0, NULL);
    assert(frame->sr.flags.vflow == 0);

    /**
     * Test comparisons.
     */

    test_compare(frame, as, &frame->ac, &INS_CMP);
    test_compare(frame, as, &frame->x, &INS_CPX);
    test_compare(frame, as, &frame->y, &INS_CPY);

    /**
     * Test conditional branch instructions.
     */

    test_branch(frame, as, &INS_BCC, SR_CARRY, 0);
    test_branch(frame, as, &INS_BCS, SR_CARRY, 1);
    test_branch(frame, as, &INS_BEQ, SR_ZERO, 1);
    test_branch(frame, as, &INS_BMI, SR_NEGATIVE, 1);
    test_branch(frame, as, &INS_BNE, SR_ZERO, 0);
    test_branch(frame, as, &INS_BPL, SR_NEGATIVE, 0);
    test_branch(frame, as, &INS_BVC, SR_OVERFLOW, 0);
    test_branch(frame, as, &INS_BVS, SR_OVERFLOW, 1);

    /**
     * Test jumps and subroutines.
     */

    const uint16_t pc_start = 1024;
    const uint16_t pc_target = pc_start + 500;

    frame->pc = pc_start;
    INS_JMP.apply(frame, as, pc_target, NULL);
    assert(frame->pc == pc_target);

    frame->sp = 255;
    frame->pc = pc_start;
    INS_JSR.apply(frame, as, pc_target, NULL);
    assert(frame->pc == pc_target);
    assert(frame->sp == 253);

    INS_RTS.apply(frame, as, 0, NULL);
    assert(frame->pc == pc_start + 3);
    assert(frame->sp == 255);

    /**
     * Test interrupts.
     */

    frame->sr.flags.brk = 0;
    INS_BRK.apply(frame, as, 0, NULL);
    assert(frame->sr.flags.brk == 1);

    /**
     * Test no-op.
     */

    tframe_t prev = *frame;
    uint8_t oldmem[UINT16_MAX];
    for (int i = 0; i < UINT16_MAX; i++) {
        oldmem[i] = *vaddr_to_ptr(as, i);
    }

    INS_NOP.apply(frame, as, 0, NULL);
    assert(prev.ac == frame->ac);
    assert(prev.pc == frame->pc);
    assert(prev.sp == frame->sp);
    assert(prev.sr.bits == frame->sr.bits);
    assert(prev.sp == frame->sp);
    assert(prev.x == frame->x);
    assert(prev.y == frame->y);
    for (int i = 0; i < UINT16_MAX; i++) {
        assert(oldmem[i] == *vaddr_to_ptr(as, i));
    }

    as_destroy(as);
}

void test_load_store(tframe_t *frame, const addrspace_t *as, uint8_t *reg, const instruction_t *load, const instruction_t *store) {
    uint8_t value;

    value = 0x00;
    load->apply(frame, as, 0, &value);
    assert(*reg == 0x00);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    value = 0x05;
    load->apply(frame, as, 0, &value);
    assert(*reg == 0x05);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    store->apply(frame, as, 0x20, NULL);
    assert(*vaddr_to_ptr(as, 0x20) == 0x05);

    value = 0xF9;
    load->apply(frame, as, 0x20, &value);
    assert(*reg == 0xF9);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    store->apply(frame, as, 0x21, NULL);
    assert(*vaddr_to_ptr(as, 0x21) == 0xF9);
}

void test_inc_dec(tframe_t *frame, const addrspace_t *as, uint8_t *value, const instruction_t *inc, const instruction_t *dec) {
    *value = 2;

    dec->apply(frame, as, 0, value);
    assert(*value == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    dec->apply(frame, as, 0, value);
    assert(*value == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    dec->apply(frame, as, 0, value);
    assert(*value == 255);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    dec->apply(frame, as, 0, value);
    assert(*value == 254);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    inc->apply(frame, as, 0, value);
    assert(*value == 255);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    inc->apply(frame, as, 0, value);
    assert(*value == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    inc->apply(frame, as, 0, value);
    assert(*value == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    *value = 129;

    dec->apply(frame, as, 0, value);
    assert(*value == 128);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    dec->apply(frame, as, 0, value);
    assert(*value == 127);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    dec->apply(frame, as, 0, value);
    assert(*value == 126);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);
    
    inc->apply(frame, as, 0, value);
    assert(*value == 127);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    inc->apply(frame, as, 0, value);
    assert(*value == 128);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    inc->apply(frame, as, 0, value);
    assert(*value == 129);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);
}

void test_compare(tframe_t *frame, const addrspace_t *as, uint8_t *reg, const instruction_t *cmp) {
    uint8_t value;

    // 8 - 5 = 3 > 0
    *reg = 0x08;
    value = 0x05;
    cmp->apply(frame, as, 0, &value);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);

    // 8 - 8 = 0
    *reg = 0x08;
    value = 0x08;
    cmp->apply(frame, as, 0, &value);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);

    // 8 - 10 = -2 < 0
    *reg = 0x08;
    value = 0x0A;
    cmp->apply(frame, as, 0, &value);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    // (-1) - (-1) = 0
    *reg = 0xFF;
    value = 0xFF;
    cmp->apply(frame, as, 0, &value);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 1);
    
    // (-1) - (-2) = 1 > 0
    *reg = 0xFF;
    value = 0xFE;
    cmp->apply(frame, as, 0, &value);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);
    
    // (-2) - (-1) = -1 < 0
    *reg = 0xFE;
    value = 0xFF;
    cmp->apply(frame, as, 0, &value);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    // (-1) - 0 = -1 < 0
    *reg = 0xFF;
    value = 0x00;
    cmp->apply(frame, as, 0, &value);
    assert(frame->sr.flags.carry == 1);
    assert(frame->sr.flags.neg == 1);
    assert(frame->sr.flags.zero == 0);

    // 0 - (-1) = 1 > 0
    *reg = 0x00;
    value = 0xFF;
    cmp->apply(frame, as, 0, &value);
    assert(frame->sr.flags.carry == 0);
    assert(frame->sr.flags.neg == 0);
    assert(frame->sr.flags.zero == 0);
}

void test_branch(tframe_t *frame, const addrspace_t *as, const instruction_t *branch, uint8_t mask, unsigned int true_val) {
    addr_t start = 1024;
    addr_t target = start + 5;
    frame->pc = start;

    frame->sr.bits = !true_val ? mask : 0x00;
    branch->apply(frame, as, target, NULL);
    assert(frame->pc == start);

    frame->sr.bits = true_val ? mask : 0x00;
    branch->apply(frame, as, target, NULL);
    assert(frame->pc == target);

    frame->pc = start;
    target = start - 5;

    frame->sr.bits = true_val ? mask : 0x00;
    branch->apply(frame, as, target, NULL);
    assert(frame->pc == target);
}
