#include <cpu.h>
#include <stdio.h>
#include <stdlib.h>

uint16_t bytes_to_word(uint8_t low, uint8_t high) {
    return (high << 8) + low;
}

uint8_t sr_to_bits(const sr_flags_t sr) {
    uint8_t bits = 0;
    if (sr.brk)
        bits |= SR_BREAK;
    if (sr.carry)
        bits |= SR_CARRY;
    if (sr.dec)
        bits |= SR_DECIMAL;
    if (sr.ign)
        bits |= SR_IGNORED;
    if (sr.irq)
        bits |= SR_INTERRUPT;
    if (sr.neg)
        bits |= SR_NEGATIVE;
    if (sr.vflow)
        bits |= SR_OVERFLOW;
    if (sr.zero)
        bits |= SR_ZERO;
    return bits;
}

sr_flags_t bits_to_sr(uint8_t bits) {
    sr_flags_t sr;
    sr.brk = (bits & SR_BREAK) > 0;
    sr.carry = (bits & SR_CARRY) > 0;
    sr.dec = (bits & SR_DECIMAL) > 0;
    sr.ign = (bits & SR_IGNORED) > 0;
    sr.irq = (bits & SR_INTERRUPT) > 0;
    sr.neg = (bits & SR_NEGATIVE) > 0;
    sr.vflow = (bits & SR_OVERFLOW) > 0;
    sr.zero = (bits & SR_ZERO) > 0;
    return sr;
}

void push(tframe_t *frame, const addrspace_t *as, uint8_t value) {
    as_write(as, STACK_START + frame->sp, value);
    frame->sp--;
}

uint8_t pull(tframe_t *frame, const addrspace_t *as) {
    frame->sp++;
    uint8_t value = as_read(as, STACK_START + frame->sp);
    return value;
}

void push_word(tframe_t *frame, const addrspace_t *as, uint16_t value) {
    push(frame, as, value >> 8);       // high
    push(frame, as, value & 0x00FF);   // low
}

uint16_t pull_word(tframe_t *frame, const addrspace_t *as) {
    uint8_t low = pull(frame, as);
    uint8_t high = pull(frame, as);
    return bytes_to_word(low, high);
}

cpu_t *cpu_create(void) {
    // Create the CPU.
    cpu_t *cpu = malloc(sizeof(struct cpu));

    // Clear registers.
    cpu->frame.ac = 0;
    cpu->frame.pc = 0;
    cpu->frame.x = 0;
    cpu->frame.y = 0;
    cpu->frame.sp = 0;

    // Initialise status register.
    cpu->frame.sr = bits_to_sr(SR_IGNORED);

    // Setup memory.
    cpu->wmem = malloc(sizeof(uint8_t) * WMEM_SIZE);
    
    // Create address space.
    cpu->as = as_create();

    // Other variables.
    cpu->joypad1 = 0;
    cpu->joypad2 = 0;
    cpu->joypad1_t = 0;
    cpu->joypad2_t = 0;
    cpu->jp_strobe = 0;
    cpu->oam_upload = 0;
    cpu->cycles = 0;

    return cpu;
}

void cpu_destroy(cpu_t *cpu) {
    // Destroy address space.
    as_destroy(cpu->as);

    // Free memory.
    free(cpu->wmem);

    // Free CPU.
    free(cpu);
}

void cpu_reset(cpu_t *cpu) {
    const uint8_t low = as_read(cpu->as, RES_VECTOR);
    const uint8_t high = as_read(cpu->as, RES_VECTOR + 1);
    cpu->frame.pc = bytes_to_word(low, high);
    cpu->frame.sr.irq = 1;
    cpu->frame.sp -= 3;
}

void cpu_nmi(cpu_t *cpu) {
    // Push PC and status register.
    push_word(&cpu->frame, cpu->as, cpu->frame.pc);
    push(&cpu->frame, cpu->as, sr_to_bits(cpu->frame.sr));
    
    // Jump to interrupt handler.
    const uint8_t low = as_read(cpu->as, NMI_VECTOR);
    const uint8_t high = as_read(cpu->as, NMI_VECTOR + 1);
    cpu->frame.pc = bytes_to_word(low, high);
}

void cpu_irq(cpu_t *cpu) {
    // Ignore if interrupt flag is set.
    if (cpu->frame.sr.irq)
        return;
    
    // Push PC and status register.
    push_word(&cpu->frame, cpu->as, cpu->frame.pc);
    push(&cpu->frame, cpu->as, sr_to_bits(cpu->frame.sr));

    // Disable further interrupts.
    cpu->frame.sr.irq = 1;
    
    // Jump to interrupt handler.
    const uint8_t low = as_read(cpu->as, IRQ_VECTOR);
    const uint8_t high = as_read(cpu->as, IRQ_VECTOR + 1);
    cpu->frame.pc = bytes_to_word(low, high);
}

uint8_t cpu_fetch(const cpu_t *cpu) {
    return as_read(cpu->as, cpu->frame.pc);
}

operation_t cpu_decode(const cpu_t *cpu, const uint8_t opc) {
    // Convert the raw opcode into an opcode_t.
    opcode_t opcode = {
        .group = opc & 0x03,
        .addrmode = (opc >> 2) & 0x07,
        .num = (opc >> 5) & 0x07,
    };

    // Decode the opcode to determine the instruction and address mode.
    operation_t result = { .opc = opc };
    result.instruction = get_instruction(opcode);
    result.addr_mode = get_address_mode(opcode);

    // Get the arguments.
    result.args[0] = as_read(cpu->as, cpu->frame.pc + 1);
    result.args[1] = as_read(cpu->as, cpu->frame.pc + 2);

    // If the instruction is invalid, then print an error and terminate.
    if (result.instruction == NULL) {
        printf("Invalid instruction: $%.2x. Program terminated.\n", opc);
        exit(1);
    }

    return result;
}

int cpu_execute(cpu_t *cpu, operation_t op) {
    // If the instruction hasn't been implemented, then print an error and terminate.
    if (op.instruction->apply == NULL) {
        printf("Instruction %s not implemented. Program terminated.\n", op.instruction->name);
        exit(1);
    }

    // Determine the memory location of the instruction's argument using the correct address mode.
    const mem_loc_t loc = op.addr_mode->resolve(&cpu->frame, cpu->as, op.args);
    
    // Execute the instruction.
    int cycles = op.instruction->apply(&cpu->frame, cpu->as, op.addr_mode, loc);

    // Advance the program counter (unless the instruction is a jump instruction).
    if (!op.instruction->jump) {
        cpu->frame.pc += op.addr_mode->argc + 1;
    }

    return cycles;
}
