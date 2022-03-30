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
    tframe_t frame = { 0 };
    uint8_t memory[MEM_SIZE] = { 0 };
    addrspace_t as = { memory };
    frame.sr.flags.ign = 1;
    frame.sp = 0xFF;

    // Load program from input.
    for (int i = 1; i < argc; i++) {
        memory[AS_PROG + i - 1] = strtol(argv[i], NULL, 16);
    }

    // Execute program.
    frame.pc = AS_PROG;
    while (frame.sr.flags.brk == 0) {
        uint8_t *insptr = fetch(&frame, &as);
        operation_t ins = decode(insptr);
        print_ins(ins);
        execute(&frame, &as, ins);
    }

    // Dump state.
    printf("Program halted.\n");
    printf("pc: 0x%.4x, a: %d. x: %d, y: %d, sp: 0x%.2x, sr: ", frame.pc, frame.ac, frame.x, frame.y, frame.sp);
    printf(frame.sr.flags.neg ? "n" : "-");
    printf(frame.sr.flags.vflow ? "v" : "-");
    printf(frame.sr.flags.ign ? "-" : "-");
    printf(frame.sr.flags.brk ? "b" : "-");
    printf(frame.sr.flags.dec ? "d" : "-");
    printf(frame.sr.flags.irq ? "i" : "-");
    printf(frame.sr.flags.zero ? "z" : "-");
    printf(frame.sr.flags.carry ? "c" : "-");

    return 0;
}
