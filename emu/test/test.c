#include <assert.h>
#include <addrmodes.h>
#include <instructions.h>
#include <stdio.h>

void exec_ins(const instruction_t *ins, tframe_t *frame, const addrspace_t *as, addr_t addr, uint8_t *value);

void test_virtual_memory(void);
void test_address_modes(tframe_t *frame);
void test_instructions(tframe_t *frame);

void test_load_store(tframe_t *frame, const addrspace_t *as, uint8_t *reg, const instruction_t *load, const instruction_t *store);
void test_inc_dec(tframe_t *frame, const addrspace_t *as, uint8_t *value, const instruction_t *inc, const instruction_t *dec);
void test_compare(tframe_t *frame, const addrspace_t *as, uint8_t *reg, const instruction_t *cmp);
void test_branch(tframe_t *frame, const addrspace_t *as, const instruction_t *branch, uint8_t mask, unsigned int true_val);

int main() {
    tframe_t frame;
    test_virtual_memory();
    test_address_modes(&frame);
    test_instructions(&frame);
    printf("All tests passed successfully!\n");
}

void exec_ins(const instruction_t *ins, tframe_t *frame, const addrspace_t *as, addr_t addr, uint8_t *value) {
    mem_loc_t loc = { addr, value };
    ins->apply(frame, as, NULL, loc);
}

void test_virtual_memory() {
    addrspace_t *as;

    uint8_t a[1024] = { 0 };
    uint8_t b[512] = { 0 };
    //uint8_t c[512] = { 0 };

    /* Test a single segment of memory. */
    as = as_create();
    as_add_segment(as, 256, 1024, a, AS_READ | AS_WRITE);

    a[0] = 5;
    a[100] = 40;
    a[500] = 90;
    a[1023] = 100;

    assert(as_read(as, 256) == 5);
    assert(as_read(as, 256 + 100) == 40);
    assert(as_read(as, 256 + 500) == 90);
    assert(as_read(as, 256 + 1023) == 100);

    as_write(as, 256 + 10, 10);
    as_write(as, 256 + 900, 20);
    as_write(as, 256 + 1023, 30);

    assert(a[10] == 10);
    assert(a[900] == 20);
    assert(a[1023] == 30);

    /* Now add another segmnet before and after. */
    as_add_segment(as, 0, 256, b, AS_READ | AS_WRITE);
    as_add_segment(as, 1280, 256, b + 256, AS_READ | AS_WRITE);

    b[0] = 1;
    b[64] = 2;
    b[255] = 3;
    b[256] = 4;
    b[400] = 5;
    b[511] = 6;

    assert(as_read(as, 0) == 1);
    assert(as_read(as, 64) == 2);
    assert(as_read(as, 255) == 3);
    assert(as_read(as, 1280) == 4);
    assert(as_read(as, 1280 + 144) == 5);
    assert(as_read(as, 1280 + 255) == 6);

    as_write(as, 10, 7);
    as_write(as, 100, 8);
    as_write(as, 255, 9);
    as_write(as, 1280, 10);
    as_write(as, 1280 + 94, 11);
    as_write(as, 1280 + 255, 12);

    assert(b[10] == 7);
    assert(b[100] == 8);
    assert(b[255] == 9);
    assert(b[256] == 10);
    assert(b[350] == 11);
    assert(b[511] == 12);

    as_destroy(as);
}

void test_address_modes(tframe_t *frame) {
    uint8_t args[3];
    uint8_t mem[1024];
    addr_t vaddr;

    // Set up a very simple 1KB address space.
    addrspace_t *as = as_create();
    as_add_segment(as, 0, 1024, mem, AS_READ | AS_WRITE);

    // Immediate.
    args[0] = 0xFF;
    assert(AM_IMMEDIATE.resolve(frame, as, args).ptr == &args[0]);
    assert(*AM_IMMEDIATE.resolve(frame, as, args).ptr == 0xFF);

    // Accumulator.
    frame->ac = 0x04;
    assert(AM_ACCUMULATOR.resolve(frame, as, args).ptr == &frame->ac);
    assert(*AM_ACCUMULATOR.resolve(frame, as, args).ptr == 0x04);

    // Zero page.
    args[0] = 0x08;
    mem[0x08] = 0x80;

    vaddr = AM_ZEROPAGE.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x08);
    assert(as_read(as, vaddr) == 0x80);

    // Zero page X.
    args[0] = 0x08;
    frame->x = 0x10;
    frame->y = 0x00;
    mem[0x18] = 0x40;

    vaddr = AM_ZEROPAGE_X.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x18);
    assert(as_read(as, vaddr) == 0x40);

    args[0] = 0xFF;
    frame->x = 0x01;
    frame->y = 0x00;
    mem[0x00] = 0x02;

    vaddr = AM_ZEROPAGE_X.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x00);
    assert(as_read(as, vaddr) == 0x02);

    // Zero page Y.
    args[0] = 0x08;
    frame->x = 0x00;
    frame->y = 0x10;
    mem[0x18] = 0x20;

    vaddr = AM_ZEROPAGE_Y.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x18);
    assert(as_read(as, vaddr) == 0x20);

    args[0] = 0xFF;
    frame->x = 0x00;
    frame->y = 0x01;
    mem[0x00] = 0x04;

    vaddr = AM_ZEROPAGE_Y.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x00);
    assert(as_read(as, vaddr) == 0x04);

    // Absolute.
    args[0] = 0xFF;
    args[1] = 0x01;
    mem[0x01FF] = 0x05;

    vaddr = AM_ABSOLUTE.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x01FF);
    assert(as_read(as, vaddr) == 0x05);

    // Absolute-X.
    args[0] = 0x05;
    args[1] = 0x01;
    frame->x = 0x01;
    frame->y = 0x00;
    mem[0x0106] = 0x06;

    vaddr = AM_ABSOLUTE_X.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x0106);
    assert(as_read(as, vaddr) == 0x06);

    // Absolute-Y.
    args[0] = 0x05;
    args[1] = 0x01;
    frame->x = 0x00;
    frame->y = 0x03;
    mem[0x0108] = 0x0A;

    vaddr = AM_ABSOLUTE_Y.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x0108);
    assert(as_read(as, vaddr) == 0x0A);

    // Indirect.
    args[0] = 0x10;
    args[1] = 0x02;
    mem[0x0210] = 0x0C;
    mem[0x0211] = 0x01;
    mem[0x010C] = 0xAF;

    vaddr = AM_INDIRECT.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x010C);
    assert(as_read(as, vaddr) == 0xAF);

    args[0] = 0xFF;
    args[1] = 0x01;
    mem[0x0100] = 0x01;
    mem[0x01FF] = 0x0D;
    mem[0x010D] = 0x8A;

    vaddr = AM_INDIRECT.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x010D);
    assert(as_read(as, vaddr) == 0x8A);

    // Indirect-X.
    args[0] = 0x10;
    frame->x = 0x02;
    frame->y = 0x00;
    mem[0x12] = 0x0A;
    mem[0x13] = 0x01;
    mem[0x010A] = 0x0B;

    vaddr = AM_INDIRECT_X.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x010A);
    assert(as_read(as, vaddr) == 0x0B);

    args[0] = 0xFF;
    frame->x = 0x01;
    frame->y = 0x00;
    mem[0x00] = 0x0B;
    mem[0x01] = 0x02;
    mem[0x020B] = 0x0C;

    vaddr = AM_INDIRECT_X.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x020B);
    assert(as_read(as, vaddr) == 0x0C);

    // Indirect-Y.
    args[0] = 0x20;
    frame->x = 0x00;
    frame->y = 0x04;
    mem[0x20] = 0x04;
    mem[0x21] = 0x01;
    mem[0x0108] = 0x0F;

    vaddr = AM_INDIRECT_Y.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x0108);
    assert(as_read(as, vaddr) == 0x0F);

    args[0] = 0xFF;
    frame->x = 0x00;
    frame->y = 0x01;
    mem[0x00] = 0x01;
    mem[0xFF] = 0x02;
    mem[0x0103] = 0x8A;

    vaddr = AM_INDIRECT_Y.resolve(frame, as, args).vaddr;
    assert(vaddr == 0x0103);
    assert(as_read(as, vaddr) == 0x8A);

    as_destroy(as);
}

void test_instructions(tframe_t *frame) {
    uint8_t value;
    uint8_t mem[65536];

    // Set up a simple address space that uses a single 64KB segment.
    addrspace_t *as = as_create();
    as_add_segment(as, 0, 65536, mem, AS_READ | AS_WRITE);

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
    exec_ins(&INS_TAX, frame, as, 0, NULL);
    assert(frame->x == frame->ac);

    frame->ac = 10;
    frame->y = 20;
    exec_ins(&INS_TAY, frame, as, 0, NULL);
    assert(frame->y == frame->ac);

    frame->x = 0x2C;
    frame->sp = 0xFF;
    exec_ins(&INS_TSX, frame, as, 0, NULL);
    assert(frame->sp == frame->x);

    /**
     * Test stack.
     */

    uint8_t sp_start = frame->sp;

    frame->ac = 10;
    exec_ins(&INS_PHA, frame, as, 0, NULL);
    assert(frame->sp = sp_start - 1);
    frame->ac = 100;
    exec_ins(&INS_PHA, frame, as, 0, NULL);
    assert(frame->sp = sp_start - 2);
    frame->ac = 200;
    exec_ins(&INS_PHA, frame, as, 0, NULL);
    assert(frame->sp = sp_start - 3);

    exec_ins(&INS_PLA, frame, as, 0, NULL);
    assert(frame->ac == 200);
    assert(frame->sp = sp_start - 2);
    exec_ins(&INS_PLA, frame, as, 0, NULL);
    assert(frame->ac == 100);
    assert(frame->sp = sp_start - 1);
    exec_ins(&INS_PLA, frame, as, 0, NULL);
    assert(frame->ac == 10);
    assert(frame->sp = sp_start);

    sr_flags_t sr = frame->sr;
    exec_ins(&INS_PHP, frame, as, 0, NULL);
    exec_ins(&INS_PLP, frame, as, 0, NULL);
    assert(frame->sr.carry == sr.carry);
    assert(frame->sr.dec == sr.dec);
    assert(frame->sr.irq == sr.irq);
    assert(frame->sr.neg == sr.neg);
    assert(frame->sr.vflow == sr.vflow);
    assert(frame->sr.zero == sr.zero);

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
    
    frame->sr.dec = 0;

    frame->ac = 0x01;
    frame->sr.carry = 0;
    value = 0x02;

    exec_ins(&INS_ADC, frame, as, 0, &value);
    assert(frame->ac == 0x03);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.vflow == 0);
    assert(frame->sr.zero == 0);

    frame->ac = 0x01;
    frame->sr.carry = 1;
    value = 0x02;

    exec_ins(&INS_ADC, frame, as, 0, &value);
    assert(frame->ac == 0x04);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.vflow == 0);
    assert(frame->sr.zero == 0);

    frame->ac = 0x7F;
    frame->sr.carry = 0;
    value = 0x01;

    exec_ins(&INS_ADC, frame, as, 0, &value);
    assert(frame->ac = 0x80);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 1);
    assert(frame->sr.vflow == 1);
    assert(frame->sr.zero == 0);

    frame->ac = 0xFF;
    frame->sr.carry = 0;
    value = 0x01;

    exec_ins(&INS_ADC, frame, as, 0, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.vflow == 0);
    assert(frame->sr.zero == 1);

    frame->ac = 0xFF;
    frame->sr.carry = 0;
    value = 0x02;

    exec_ins(&INS_ADC, frame, as, 0, &value);
    assert(frame->ac == 0x01);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.vflow == 0);
    assert(frame->sr.zero == 0);

    frame->ac = 0xFF;
    frame->sr.carry = 1;
    value = 0xFF;

    exec_ins(&INS_ADC, frame, as, 0, &value);
    assert(frame->ac == 0xFF);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 1);
    assert(frame->sr.vflow == 0);
    assert(frame->sr.zero == 0);

    frame->ac = 0x80;
    frame->sr.carry = 0;
    value = 0xFF;

    exec_ins(&INS_ADC, frame, as, 0, &value);
    assert(frame->ac == 0x7F);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.vflow == 1);
    assert(frame->sr.zero == 0);

    // SBC

    frame->sr.dec = 0;

    frame->ac = 0x08;
    frame->sr.carry = 1;
    value = 0x02;

    exec_ins(&INS_SBC, frame, as, 0, &value);
    assert(frame->ac == 0x06);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.vflow == 0);
    assert(frame->sr.zero == 0);

    frame->ac = 0x08;
    frame->sr.carry = 1;
    value = 0x08;

    exec_ins(&INS_SBC, frame, as, 0, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.vflow == 0);
    assert(frame->sr.zero == 1);

    frame->ac = 0x08;
    frame->sr.carry = 1;
    value = 0x09;

    exec_ins(&INS_SBC, frame, as, 0, &value);
    assert(frame->ac == 0xFF);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 1);
    assert(frame->sr.vflow == 0);
    assert(frame->sr.zero == 0);

    frame->ac = 0x08;
    frame->sr.carry = 0;
    value = 0x02;

    exec_ins(&INS_SBC, frame, as, 0, &value);
    assert(frame->ac == 0x05);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.vflow == 0);
    assert(frame->sr.zero == 0);

    frame->ac = 0x81;
    frame->sr.carry = 1;
    value = 0x02;

    exec_ins(&INS_SBC, frame, as, 0, &value);
    assert(frame->ac == 0x7F);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.vflow == 1);
    assert(frame->sr.zero == 0);

    frame->ac = 0x85;
    frame->sr.carry = 1;
    value = 0x02;

    exec_ins(&INS_SBC, frame, as, 0, &value);
    assert(frame->ac == 0x83);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 1);
    assert(frame->sr.vflow == 0);
    assert(frame->sr.zero == 0);

    frame->ac = 0x00;
    frame->sr.carry = 1;
    value = 0xFF;

    exec_ins(&INS_SBC, frame, as, 0, &value);
    assert(frame->ac == 0x01);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.vflow == 0);
    assert(frame->sr.zero == 0);

    frame->ac = 0x7F;
    frame->sr.carry = 1;
    value = 0xFF;

    exec_ins(&INS_SBC, frame, as, 0, &value);
    assert(frame->ac == 0x80);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 1);
    assert(frame->sr.vflow == 1);
    assert(frame->sr.zero == 0);

    /**
     * Test logical operators.
     */

    // AND

    frame->ac = 0xF0;
    value = 0x0F;
    exec_ins(&INS_AND, frame, as, 0, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.zero == 1);
    assert(frame->sr.neg == 0);

    frame->ac = 0x03;
    value = 0x06;
    exec_ins(&INS_AND, frame, as, 0, &value);
    assert(frame->ac == 0x02);
    assert(frame->sr.zero == 0);
    assert(frame->sr.neg == 0);

    frame->ac = 0xF0;
    value = 0xF0;
    exec_ins(&INS_AND, frame, as, 0, &value);
    assert(frame->ac == 0xF0);
    assert(frame->sr.zero == 0);
    assert(frame->sr.neg == 1);

    // OR

    frame->ac = 0xF0;
    value = 0x0F;
    exec_ins(&INS_ORA, frame, as, 0, &value);
    assert(frame->ac == 0xFF);
    assert(frame->sr.zero == 0);
    assert(frame->sr.neg == 1);

    frame->ac = 0x03;
    value = 0x06;
    exec_ins(&INS_ORA, frame, as, 0, &value);
    assert(frame->ac == 0x07);
    assert(frame->sr.zero == 0);
    assert(frame->sr.neg == 0);

    frame->ac = 0x00;
    value = 0x00;
    exec_ins(&INS_ORA, frame, as, 0, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.zero == 1);
    assert(frame->sr.neg == 0);

    // XOR

    frame->ac = 0xF0;
    value = 0x0F;
    exec_ins(&INS_EOR, frame, as, 0, &value);
    assert(frame->ac == 0xFF);
    assert(frame->sr.zero == 0);
    assert(frame->sr.neg == 1);

    frame->ac = 0x03;
    value = 0x06;
    exec_ins(&INS_EOR, frame, as, 0, &value);
    assert(frame->ac == 0x05);
    assert(frame->sr.zero == 0);
    assert(frame->sr.neg == 0);

    frame->ac = 0xF0;
    value = 0xF0;
    exec_ins(&INS_EOR, frame, as, 0, &value);
    assert(frame->ac == 0x00);
    assert(frame->sr.zero == 1);
    assert(frame->sr.neg == 0);

    /**
     * Test shift and rotate instructions.
     */

    // ASL (left shift).

    frame->ac = 0x20;
    frame->sr.carry = 1;
    frame->sr.neg = 1;
    frame->sr.zero = 1;

    exec_ins(&INS_ASL, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x40);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);

    exec_ins(&INS_ASL, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x80);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);

    exec_ins(&INS_ASL, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 1);

    exec_ins(&INS_ASL, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 1);

    // LSR (right shift).

    frame->ac = 0x02;
    frame->sr.carry = 1;
    frame->sr.neg = 1;
    frame->sr.zero = 1;

    exec_ins(&INS_LSR, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x01);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);

    exec_ins(&INS_LSR, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 1);

    exec_ins(&INS_LSR, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 1);

    frame->ac = 0x90;
    frame->sr.carry = 1;
    frame->sr.neg = 1;
    frame->sr.zero = 1;

    exec_ins(&INS_LSR, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x48);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);

    // ROL (rotate left).

    frame->ac = 0x41;
    frame->sr.carry = 0;
    frame->sr.neg = 0;
    frame->sr.zero = 0;

    exec_ins(&INS_ROL, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x82);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);

    exec_ins(&INS_ROL, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x04);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);

    frame->ac = 0x00;
    frame->sr.carry = 0;
    
    exec_ins(&INS_ROL, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 1);

    // ROR (rotate right).

    frame->ac = 0x05;
    frame->sr.carry = 0;
    frame->sr.neg = 0;
    frame->sr.zero = 0;

    exec_ins(&INS_ROR, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x02);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);

    exec_ins(&INS_ROR, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x81);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);

    frame->ac = 0x00;
    frame->sr.carry = 0;

    exec_ins(&INS_ROR, frame, as, 0, &frame->ac);
    assert(frame->ac == 0x00);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 1);

    /**
     * Test flag instructions (set and clear).
     */

    frame->sr.carry = 0;
    frame->sr.dec = 0;
    frame->sr.irq = 0;
    frame->sr.vflow = 1;

    exec_ins(&INS_SEC, frame, as, 0, NULL);
    assert(frame->sr.carry == 1);
    exec_ins(&INS_SED, frame, as, 0, NULL);
    assert(frame->sr.dec == 1);
    exec_ins(&INS_SEI, frame, as, 0, NULL);
    assert(frame->sr.irq == 1);

    exec_ins(&INS_CLC, frame, as, 0, NULL);
    assert(frame->sr.carry == 0);
    exec_ins(&INS_CLD, frame, as, 0, NULL);
    assert(frame->sr.dec == 0);
    exec_ins(&INS_CLI, frame, as, 0, NULL);
    assert(frame->sr.irq == 0);
    exec_ins(&INS_CLV, frame, as, 0, NULL);
    assert(frame->sr.vflow == 0);

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
    exec_ins(&INS_JMP, frame, as, pc_target, NULL);
    assert(frame->pc == pc_target);

    frame->sp = 255;
    frame->pc = pc_start;
    exec_ins(&INS_JSR, frame, as, pc_target, NULL);
    assert(frame->pc == pc_target);
    assert(frame->sp == 253);

    exec_ins(&INS_RTS, frame, as, 0, NULL);
    assert(frame->pc == pc_start + 3);
    assert(frame->sp == 255);

    /**
     * Test interrupts.
     */
    
    uint8_t old_sp = frame->sp;
    exec_ins(&INS_BRK, frame, as, 0, NULL);
    assert(frame->sp == old_sp - 3);

    /**
     * Test no-op.
     */

    tframe_t prev = *frame;
    uint8_t oldmem[65536];
    for (int i = 0; i < 65536; i++) {
        oldmem[i] = as_read(as, i);
    }

    exec_ins(&INS_NOP, frame, as, 0, NULL);
    assert(prev.ac == frame->ac);
    assert(prev.pc == frame->pc);
    assert(prev.sp == frame->sp);
    assert(prev.sr.brk == frame->sr.brk);
    assert(prev.sr.carry == frame->sr.carry);
    assert(prev.sr.dec == frame->sr.dec);
    assert(prev.sr.ign == frame->sr.ign);
    assert(prev.sr.irq == frame->sr.irq);
    assert(prev.sr.neg == frame->sr.neg);
    assert(prev.sr.vflow == frame->sr.vflow);
    assert(prev.sr.zero == frame->sr.zero);
    assert(prev.sp == frame->sp);
    assert(prev.x == frame->x);
    assert(prev.y == frame->y);
    for (int i = 0; i < 65536; i++) {
        assert(oldmem[i] == as_read(as, i));
    }

    as_destroy(as);
}

void test_load_store(tframe_t *frame, const addrspace_t *as, uint8_t *reg, const instruction_t *load, const instruction_t *store) {
    uint8_t value;

    value = 0x00;
    exec_ins(load, frame, as, 0, &value);
    assert(*reg == 0x00);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 1);
    
    as_write(as, 0x19, 0x05);
    exec_ins(load, frame, as, 0x19, NULL);
    assert(*reg == 0x05);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);

    exec_ins(store, frame, as, 0x20, NULL);
    assert(as_read(as, 0x20) == 0x05);

    value = 0xF9;
    exec_ins(load, frame, as, 0x20, &value);
    assert(*reg == 0xF9);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);

    exec_ins(store, frame, as, 0x21, NULL);
    assert(as_read(as, 0x21) == 0xF9);
}

void test_inc_dec(tframe_t *frame, const addrspace_t *as, uint8_t *value, const instruction_t *inc, const instruction_t *dec) {
    *value = 2;

    exec_ins(dec, frame, as, 0, value);
    assert(*value == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);

    exec_ins(dec, frame, as, 0, value);
    assert(*value == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 1);

    exec_ins(dec, frame, as, 0, value);
    assert(*value == 255);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);

    exec_ins(dec, frame, as, 0, value);
    assert(*value == 254);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);

    exec_ins(inc, frame, as, 0, value);
    assert(*value == 255);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);

    exec_ins(inc, frame, as, 0, value);
    assert(*value == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 1);

    exec_ins(inc, frame, as, 0, value);
    assert(*value == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);

    *value = 129;

    exec_ins(dec, frame, as, 0, value);
    assert(*value == 128);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);

    exec_ins(dec, frame, as, 0, value);
    assert(*value == 127);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);

    exec_ins(dec, frame, as, 0, value);
    assert(*value == 126);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);
    
    exec_ins(inc, frame, as, 0, value);
    assert(*value == 127);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);

    exec_ins(inc, frame, as, 0, value);
    assert(*value == 128);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);

    exec_ins(inc, frame, as, 0, value);
    assert(*value == 129);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);
}

void test_compare(tframe_t *frame, const addrspace_t *as, uint8_t *reg, const instruction_t *cmp) {
    uint8_t value;

    // 8 - 5 = 3 > 0
    *reg = 0x08;
    value = 0x05;
    exec_ins(cmp, frame, as, 0, &value);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);

    // 8 - 8 = 0
    *reg = 0x08;
    value = 0x08;
    exec_ins(cmp, frame, as, 0, &value);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 1);

    // 8 - 10 = -2 < 0
    *reg = 0x08;
    value = 0x0A;
    exec_ins(cmp, frame, as, 0, &value);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);

    // (-1) - (-1) = 0
    *reg = 0xFF;
    value = 0xFF;
    exec_ins(cmp, frame, as, 0, &value);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 1);
    
    // (-1) - (-2) = 1 > 0
    *reg = 0xFF;
    value = 0xFE;
    exec_ins(cmp, frame, as, 0, &value);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);
    
    // (-2) - (-1) = -1 < 0
    *reg = 0xFE;
    value = 0xFF;
    exec_ins(cmp, frame, as, 0, &value);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);

    // (-1) - 0 = -1 < 0
    *reg = 0xFF;
    value = 0x00;
    exec_ins(cmp, frame, as, 0, &value);
    assert(frame->sr.carry == 1);
    assert(frame->sr.neg == 1);
    assert(frame->sr.zero == 0);

    // 0 - (-1) = 1 > 0
    *reg = 0x00;
    value = 0xFF;
    exec_ins(cmp, frame, as, 0, &value);
    assert(frame->sr.carry == 0);
    assert(frame->sr.neg == 0);
    assert(frame->sr.zero == 0);
}

void test_branch(tframe_t *frame, const addrspace_t *as, const instruction_t *branch, uint8_t mask, unsigned int true_val) {
    addr_t start = 1024;
    addr_t target = start + 5;
    frame->pc = start;

    frame->sr = bits_to_sr(!true_val ? mask : 0x00);
    exec_ins(branch, frame, as, target, NULL);
    assert(frame->pc == start);

    frame->sr = bits_to_sr(true_val ? mask : 0x00);
    exec_ins(branch, frame, as, target, NULL);
    assert(frame->pc == target);

    frame->pc = start;
    target = start - 5;

    frame->sr = bits_to_sr(true_val ? mask : 0x00);
    exec_ins(branch, frame, as, target, NULL);
    assert(frame->pc == target);
}
