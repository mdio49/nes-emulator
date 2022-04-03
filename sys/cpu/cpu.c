#include <cpu.h>
#include <stdio.h>
#include <stdlib.h>

uint16_t bytes_to_word(uint8_t low, uint8_t high) {
    return (high << 8) + low;
}

cpu_t *cpu_create(void) {
    // Create CPU struct.
    cpu_t *cpu = malloc(sizeof(struct cpu));

    // Clear registers.
    cpu->frame.ac = 0;
    cpu->frame.pc = 0;
    cpu->frame.sr.bits = 0;
    cpu->frame.sr.flags.ign = 1;
    cpu->frame.x = 0;
    cpu->frame.y = 0;

    // Setup memory.
    cpu->wmem = malloc(sizeof(uint8_t) * WMEM_SIZE);

    // Setup stack.
    cpu->frame.sp = 0xFF;
    cpu->stack = cpu->wmem + STACK_START;

    // Setup address space.
    cpu->as = as_create();
    for (int i = 0; i < 4; i++) {
        as_add_segment(cpu->as, i * WMEM_SIZE, WMEM_SIZE, cpu->wmem);
    }
    for (int i = 0x2000; i < 0x4000; i += 8) {
        as_add_segment(cpu->as, i, 8, cpu->ppu_reg);
    }
    as_add_segment(cpu->as, 0x4000, 0x0020, cpu->apu_io_reg);

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
    uint8_t low = *as_resolve(cpu->as, RES_VECTOR);
    uint8_t high = *as_resolve(cpu->as, RES_VECTOR + 1);
    cpu->frame.pc = bytes_to_word(low, high);
}

uint8_t *cpu_fetch(const cpu_t *cpu) {
    return as_resolve(cpu->as, cpu->frame.pc);
}

operation_t cpu_decode(const cpu_t *cpu, const uint8_t *insptr) {
    // Get the opcode and args from the instruction pointer.
    const uint8_t opc = *insptr;

    // Convert the raw data into an opcode_t.
    opcode_converter_t decoder;
    decoder.raw = opc;

    // Decode the opcode to determine the instruction and address mode.
    operation_t result;
    result.instruction = get_instruction(decoder.opcode);
    result.addr_mode = get_address_mode(decoder.opcode);

    // Get the arguments.
    result.args[0] = *as_resolve(cpu->as, cpu->frame.pc + 1);
    result.args[1] = *as_resolve(cpu->as, cpu->frame.pc + 2);

    // If the instruction is invalid, then print an error and terminate.
    if (result.instruction == NULL) {
        printf("Invalid instruction: $%.2x. Program terminated.\n", decoder.raw);
        exit(1);
    }

    return result;
}

void cpu_execute(cpu_t *cpu, operation_t op) {
    // If the instruction hasn't been implemented, then print an error and terminate.
    if (op.instruction->apply == NULL) {
        printf("Instruction %s not implemented. Program terminated.\n", op.instruction->name);
        exit(1);
    }

    // Evaluate the value of the instruction's argument(s) using the correct address mode.
    const vaddr_ptr_pair_t pair = op.addr_mode->evaluate(&cpu->frame, cpu->as, op.args);

    // Execute the instruction.
    op.instruction->apply(&cpu->frame, cpu->as, pair.vaddr, pair.ptr);

    // Advance the program counter (unless the instruction is a jump instruction).
    if (!op.instruction->jump) {
        cpu->frame.pc += op.addr_mode->argc + 1;
    }
}
