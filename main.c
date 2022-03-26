#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cpu.h"

void print_ins(operation_t ins) {
    printf("%s", ins.instruction->name);
    int argc = ins.addr_mode != NULL ? ins.addr_mode->argc : 1;
    for (int i = 0; i < argc; i++) {
        printf(" $%.2x", ins.args[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    // Setup CPU.
    tframe_t frame;
    uint8_t memory[MEM_SIZE] = { 0 };
    frame.sr.flag_ignored = 1;

    // Load program from input.
    for (int i = 1; i < argc; i++) {
        memory[AS_PROG + i - 1] = strtol(argv[i], NULL, 16);
    }

    // Execute program.
    frame.pc = AS_PROG;
    while (memory[frame.pc] != 0x00) {
        uint8_t *insptr = fetch(&frame, memory);
        operation_t ins = decode(insptr);
        print_ins(ins);
        execute(&frame, memory, ins);
    }

    // Dump state.
    printf("pc: 0x%.4x, a: %d. x: %d, y: %d, sp: %d\n", frame.pc, frame.ac, frame.x, frame.y, frame.sp);

    return 0;
}
