#include <addrmodes.h>
#include <cpu.h>
#include <prog.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define HIST_LEN 50

void exit_handler(void);
void keyboard_interupt_handler(int signum);

void run_bin(const char *path);
void run_hex(int argc, char *argv[]);
void run_test(const char *path);

const char *load_rom(const char *path);

void print_ins(operation_t ins);
bool strprefix(const char *str, const char *pre);

static cpu_t *cpu = NULL;
static bool interrupted = false;
static operation_t history[HIST_LEN] = { 0 };

int main(int argc, char *argv[]) {
    // Setup signal handlers.
    atexit(exit_handler);

    // Setup CPU.
    cpu = cpu_create();

    // Parse CL arguments.
    if (strcmp(argv[1], "-x") == 0) {
        run_hex(argc - 2, argv + 2);
    }
    else if (strcmp(argv[1], "-t") == 0) {
        run_test(argv[2]);
    }
    else {
        run_bin(argv[1]);
    }
    return 0;
}

void exit_handler() {
    if (cpu == NULL)
        return;

    // Print history.
    printf("----------\n");
    for (int i = 0; i < HIST_LEN; i++) {
        if (history[i].instruction != NULL) {
            print_ins(history[i]);
        }
    }
    printf("----------\n");

    // Dump state.
    printf("pc: $%.4x, a: %d. x: %d, y: %d, sp: $%.2x, sr: ", cpu->frame.pc, cpu->frame.ac, cpu->frame.x, cpu->frame.y, cpu->frame.sp);
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
}

void keyboard_interupt_handler(int signum) {
    char buffer[50];
    interrupted = true;
    while (interrupted) {
        printf("> ");
        scanf("%49s", buffer);
        if (strprefix(buffer, "reset")) {
            cpu_reset(cpu);
            interrupted = false;
        }
        else if (strprefix(buffer, "quit")) {
            exit(0);
        }
        else if (strprefix(buffer, "print")) {
            char c;
            int i = 0x6004;
            while ((c = *vaddr_to_ptr(cpu->as, i)) != '\0') {
                putchar(c);
                i++;
            }
            putchar('\n');
        }
        else if (strprefix(buffer, "resume")) {
            interrupted = false;
        }
        else {
            printf("Invalid command.\n");
        }
    }

    signal(SIGINT, keyboard_interupt_handler);
}

void run_bin(const char *path) {
    // Setup signal handlers.
    signal(SIGINT, keyboard_interupt_handler);

    // Load the program.
    const char *src = load_rom(path);
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
    cpu_reset(cpu);
    while (true) {
        while (interrupted) { }
        uint8_t *insptr = cpu_fetch(cpu);
        operation_t ins = cpu_decode(insptr);
        printf("$%.4x: ", cpu->frame.pc);
        print_ins(ins);
        cpu_execute(cpu, ins);
    }
    
    // Destroy the program.
    prog_destroy(prog);
}

void run_test(const char *path) {
    // Load the program.
    const char *src = load_rom(path);
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
    int msg_ptr = 0x6004;
    cpu_reset(cpu);
    while (true) {
        // Execute the next instruction.
        uint8_t *insptr = cpu_fetch(cpu);
        operation_t ins = cpu_decode(insptr);
        cpu_execute(cpu, ins);

        // Update history.
        for (int i = 0; i < HIST_LEN - 1; i++) {
            history[i] = history[i + 1];
        }
        history[HIST_LEN - 1] = ins;

        // Display a message if available.
        char *msg = (char *)vaddr_to_ptr(cpu->as, msg_ptr);
        if (*msg != '\0') {
            printf("%s\n", msg);
            msg_ptr += strlen(msg);
        }
    }
}

void run_hex(int argc, char *bytes[]) {
    // Load program from input.
    for (int i = 0; i < argc; i++) {
        *vaddr_to_ptr(cpu->as, AS_PROG + i) = strtol(bytes[i], NULL, 16);
    }

    // Execute program.
    cpu->frame.pc = AS_PROG;
    while (cpu->frame.sr.flags.brk == 0) {
        uint8_t *insptr = cpu_fetch(cpu);
        operation_t ins = cpu_decode(insptr);
        printf("$%.4x: ", cpu->frame.pc);
        print_ins(ins);
        cpu_execute(cpu, ins);
    }

    printf("Program halted.\n");
}

const char *load_rom(const char *path) {
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

    return src;
}

void print_ins(operation_t ins) {
    printf("%s", ins.instruction->name);
    if (ins.addr_mode == &AM_IMMEDIATE) {
        printf(" #$%.2x", ins.args[0]);
    }
    else if (ins.addr_mode == &AM_ACCUMULATOR) {
        printf(" A");
    }
    else if (ins.addr_mode == &AM_ZEROPAGE || ins.addr_mode == &AM_RELATIVE) {
        printf(" $%.2x", ins.args[0]);
    }
    else if (ins.addr_mode == &AM_ZEROPAGE_X) {
        printf(" $%.2x,X", ins.args[0]);
    }
    else if (ins.addr_mode == &AM_ZEROPAGE_Y) {
        printf(" $%.2x,Y", ins.args[0]);
    }
    else if (ins.addr_mode == &AM_ABSOLUTE) {
        printf(" $%.2x%.2x", ins.args[1], ins.args[0]);
    }
    else if (ins.addr_mode == &AM_ABSOLUTE_X) {
        printf(" $%.2x%.2x,X", ins.args[1], ins.args[0]);
    }
    else if (ins.addr_mode == &AM_ABSOLUTE_Y) {
        printf(" $%.2x%.2x,Y", ins.args[1], ins.args[0]);
    }
    else if (ins.addr_mode == &AM_INDIRECT) {
        printf(" ($%.2x%.2x)", ins.args[1], ins.args[0]);
    }
    else if (ins.addr_mode == &AM_INDIRECT_X) {
        printf(" ($%.2x,X)", ins.args[0]);
    }
    else if (ins.addr_mode == &AM_INDIRECT_X) {
        printf(" ($%.2x),Y", ins.args[0]);
    }
    printf("\n");
}

bool strprefix(const char *str, const char *pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}
