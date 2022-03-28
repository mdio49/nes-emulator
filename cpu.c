#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>

uint16_t word(uint8_t low, uint8_t high) {
    return (high << 8) + low;
}

uint8_t *fetch(const tframe_t *frame, uint8_t *mem) {
    return mem + frame->pc;
}

operation_t decode(const uint8_t *insptr) {
    // Get the opcode and args from the instruction pointer.
    const uint8_t opc = *insptr;
    const uint8_t *args = insptr + 1;

    // Convert the raw data into an opcode_t.
    opcode_converter_t decoder;
    decoder.raw = opc;

    // Decode the opcode to determine the instruction and address mode.
    operation_t result;
    result.instruction = get_instruction(decoder.opcode);
    result.addr_mode = get_address_mode(decoder.opcode);
    result.args = args;

    // If the instruction is invalid, then print an error and terminate.
    if (result.instruction == NULL) {
        printf("Invalid instruction: %d. Program terminated.\n", decoder.raw);
        exit(1);
    }

    return result;
}

void execute(tframe_t *frame, uint8_t *mem, operation_t op) {
    // If the instruction hasn't been implemented, then print an error and terminate.
    if (op.instruction->apply == NULL) {
        printf("Instruction %s not implemented. Program terminated.\n", op.instruction->name);
        exit(1);
    }

    // Evaluate the value of the instruction's argument(s) using the correct address mode.
    uint8_t *value = (uint8_t *)op.addr_mode->evaluate(frame, mem, op.args);

    // Advance the program counter first to make jumping instructions easier.
    frame->pc += op.addr_mode->argc + 1;

    // Execute the instruction.
    op.instruction->apply(frame, mem, value);
}
