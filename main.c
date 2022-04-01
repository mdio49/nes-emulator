#include <cpu.h>
#include <prog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void run_bin(cpu_t *cpu, const char *path);
void run_hex(cpu_t *cpu, int argc, char *argv[]);

void print_ins(operation_t ins);

int main(int argc, char *argv[]) {
    // Setup CPU.
    cpu_t *cpu = cpu_create();

    // Parse CL arguments.
    if (strcmp(argv[1], "-x") == 0) {
        run_hex(cpu, argc - 2, argv + 2);
    }
    else {
        run_bin(cpu, argv[1]);
    }

    // Dump state.
    printf("Program halted.\n");
    printf("pc: 0x%.4x, a: %d. x: %d, y: %d, sp: 0x%.2x, sr: ", cpu->frame.pc, cpu->frame.ac, cpu->frame.x, cpu->frame.y, cpu->frame.sp);
    printf(cpu->frame.sr.flags.neg ? "n" : "-");
    printf(cpu->frame.sr.flags.vflow ? "v" : "-");
    printf(cpu->frame.sr.flags.ign ? "-" : "-");
    printf(cpu->frame.sr.flags.brk ? "b" : "-");
    printf(cpu->frame.sr.flags.dec ? "d" : "-");
    printf(cpu->frame.sr.flags.irq ? "i" : "-");
    printf(cpu->frame.sr.flags.zero ? "z" : "-");
    printf(cpu->frame.sr.flags.carry ? "c" : "-");

    // Destroy CPU.
    cpu_destroy(cpu);

    return 0;
}

void run_bin(cpu_t *cpu, const char *path) {
    // Open the ROM.
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Unable to open ROM.");
        exit(1);
    }

    // Get the length of the file.
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    // Allocate memory for the buffer and read in the data.
    char *src = malloc(size);
    fread(src, size, 1, fp);
    fclose(fp);

    // Load the program.
    prog_t *prog = prog_create(src);
    if (prog == NULL) {
        fprintf(stderr, "Unable to load ROM.");
        exit(1);
    }

    // Load the ROM into memory.
    for (int i = 0; i < prog->header.prg_rom_size * INES_PRG_ROM_UNIT; i++) {
        *vaddr_to_ptr(cpu->as, i + AS_PROG) = prog->prg_rom[i];
    }

    // Execute program.
    uint8_t low = *vaddr_to_ptr(cpu->as, 0xFFFC);
    uint8_t high = *vaddr_to_ptr(cpu->as, 0xFFFD);
    cpu->frame.pc = bytes_to_word(low, high);
    while (cpu->frame.sr.flags.brk == 0) {
        uint8_t *insptr = cpu_fetch(cpu);
        operation_t ins = cpu_decode(insptr);
        printf("0x%.4x: ", cpu->frame.pc);
        print_ins(ins);
        cpu_execute(cpu, ins);
    }
    
    // Destroy the program.
    prog_destroy(prog);
}

void run_hex(cpu_t *cpu, int argc, char *bytes[]) {
    // Load program from input.
    for (int i = 0; i < argc; i++) {
        *vaddr_to_ptr(cpu->as, AS_PROG + i) = strtol(bytes[i], NULL, 16);
    }

    // Execute program.
    cpu->frame.pc = AS_PROG;
    while (cpu->frame.sr.flags.brk == 0) {
        uint8_t *insptr = cpu_fetch(cpu);
        operation_t ins = cpu_decode(insptr);
        printf("0x%.4x: ", cpu->frame.pc);
        print_ins(ins);
        cpu_execute(cpu, ins);
    }
}

void print_ins(operation_t ins) {
    printf("%s", ins.instruction->name);
    int argc = ins.addr_mode != NULL ? ins.addr_mode->argc : 1;
    for (int i = 0; i < argc; i++) {
        printf(" $%.2x", ins.args[i]);
    }
    printf("\n");
}
