#include <addrmodes.h>
#include <cpu.h>
#include <ppu.h>
#include <prog.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include SDL.
#define SDL_MAIN_HANDLED
#include <sdl.h>

#define HIST_LEN 50

typedef struct history {

    operation_t     op;
    uint16_t        pc;

} history_t;

void run_bin(const char *path, bool test);
void run_hex(int argc, char *argv[]);

bool init();
void exit_handler(void);
void keyboard_interupt_handler(int signum);

const char *load_rom(const char *path);

void dump_state();
void print_hist();
void print_ins(operation_t ins);

bool strprefix(const char *str, const char *pre);

static SDL_Window *mainWindow = NULL;
static SDL_Surface *mainSurface = NULL;
static SDL_Renderer *mainRenderer = NULL;

static cpu_t *cpu = NULL;
static ppu_t *ppu = NULL;
static bool interrupted = false;
static history_t history[HIST_LEN] = { 0 };

int main(int argc, char *argv[]) {
    // Setup signal handlers.
    signal(SIGINT, keyboard_interupt_handler);
    atexit(exit_handler);

    // Setup CPU.
    cpu = cpu_create();

    // Setup PPU.
    ppu = ppu_create();

    // Parse CL arguments.
    bool test = false;
    bool nogui = false;
    char *path = NULL;
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (strcmp(arg, "-x") == 0) {
            run_hex(argc - (i + 1), argv + i + 1);
            return EXIT_SUCCESS;
        }
        else if (strcmp(arg, "-t") == 0) {
            test = true;
        }
        else if (strcmp(arg, "-nogui") == 0) {
            nogui = true;
        }
        else if (!strprefix(arg, "-") && path == NULL) {
            path = arg;
        }
        else {
            printf("Usage: %s [<path|-x hex...>] [-t] [-nogui]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    
    if (path == NULL) {
        printf("Usage: %s [<path|-x hex...>] [-t] [-nogui]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!nogui && !init()) {
		return EXIT_FAILURE;
    }

    run_bin(path, test);
    return EXIT_SUCCESS;
}

void run_bin(const char *path, bool test) {
    uint8_t mapper_regs[0x1FE0];
    uint8_t prg_ram[0x2000];

    bool started = false;
    int status = 0x00;
    int msg_ptr = 0x6004;

    // Load the program.
    const char *src = load_rom(path);
    prog_t *prog = prog_create(src);
    if (prog == NULL) {
        fprintf(stderr, "Unable to load ROM.");
        exit(1);
    }

    // Setup CPU address space.
    as_add_segment(cpu->as, 0x4020, 0x1FE0, mapper_regs);
    as_add_segment(cpu->as, 0x6000, 0x2000, prg_ram);
    as_add_segment(cpu->as, 0x8000, 0x4000, (uint8_t*)prog->prg_rom);
    as_add_segment(cpu->as, 0xC000, 0x4000, (uint8_t*)prog->prg_rom + 0x4000);

    // Setup PPU address space.
    as_add_segment(ppu->as, 0x0000, 0x2000, (uint8_t*)prog->chr_rom);
    as_add_segment(ppu->as, 0x2000, 0x0400, ppu->vram);
    as_add_segment(ppu->as, 0x3000, 0x0400, ppu->vram);
    if (prog->header.mirroring == 1) {
        // Vertical mirroring.
        as_add_segment(ppu->as, 0x2400, 0x0400, ppu->vram + 0x0400);
        as_add_segment(ppu->as, 0x2800, 0x0400, ppu->vram);
        as_add_segment(ppu->as, 0x2C00, 0x0400, ppu->vram + 0x0400);

        as_add_segment(ppu->as, 0x3400, 0x0400, ppu->vram + 0x0400);
        as_add_segment(ppu->as, 0x3800, 0x0400, ppu->vram);
        as_add_segment(ppu->as, 0x3C00, 0x0300, ppu->vram + 0x0400);
    }
    else {
        // Horizontal mirroring.
        as_add_segment(ppu->as, 0x2400, 0x0400, ppu->vram);
        as_add_segment(ppu->as, 0x2800, 0x0400, ppu->vram + 0x0400);
        as_add_segment(ppu->as, 0x2C00, 0x0400, ppu->vram + 0x0400);

        as_add_segment(ppu->as, 0x3400, 0x0400, ppu->vram);
        as_add_segment(ppu->as, 0x3800, 0x0400, ppu->vram + 0x0400);
        as_add_segment(ppu->as, 0x3C00, 0x0300, ppu->vram + 0x0400);
    }

    // Palette memory.
    for (int i = 0; i < 8; i++) {
        addr_t offset = 0x3F00 + (i * 0x0020);
        as_add_segment(ppu->as, offset, 1, &ppu->bkg_color);
        for (int j = 0; j < N_PALETTES; j++) {
            as_add_segment(ppu->as, offset + 1 + j * 4, 3, (uint8_t*)&ppu->palettes[j]);
            as_add_segment(ppu->as, offset + (j + 1) * 4, 1, &ppu->bkg_color);
        }
    }

    // Execute program.
    cpu_reset(cpu);
    while (!started || status >= 0x80) {
        // Spin while an interrupt is taking place.
        while (interrupted) { }

        // Fetch and decode the next instruction.
        uint8_t *insptr = cpu_fetch(cpu);
        operation_t ins = cpu_decode(cpu, insptr);

        // Update history.
        for (int i = 0; i < HIST_LEN - 1; i++) {
            history[i] = history[i + 1];
        }
        history[HIST_LEN - 1].pc = cpu->frame.pc;
        history[HIST_LEN - 1].op = ins;

        // Execute the instruction.
        cpu_execute(cpu, ins);
        
        if (test) {
            // Display a message if available.
            char *msg = (char *)as_resolve(cpu->as, msg_ptr);
            if (*msg != '\0') {
                printf("%s", msg);
                msg_ptr += strlen(msg);
            }

            // Update status of test.
            int new_status = *as_resolve(cpu->as, 0x6000);
            if (new_status != status) {
                switch (new_status) {
                    case 0x80:
                        printf("Test running...\n");
                        started = true;
                        break;
                    case 0x81:
                        printf("Reset required.\n");
                        break;
                    default:
                        printf("Test completed with result code %d.\n", new_status);
                        break;
                }
                status = new_status;
            }
        }
    }
}

void run_hex(int argc, char *bytes[]) {
    // Starting address; consistent with easy 6502.
    const addr_t start = 0x0600;

    // Setup an address space that uses a single 64KB segment.
    uint8_t mem[65536];
    addrspace_t *as = as_create();
    as_add_segment(as, 0, 65536, mem);
    as_destroy(cpu->as);
    cpu->as = as;

    // Load program from input.
    for (int i = 0; i < argc; i++) {
        *as_resolve(cpu->as, start + i) = strtol(bytes[i], NULL, 16);
    }

    // Execute program.
    addr_t prev_pc = 0;
    cpu->frame.pc = start;
    while (cpu->frame.sr.flags.brk == 0) {
        prev_pc = cpu->frame.pc;
        uint8_t *insptr = cpu_fetch(cpu);
        operation_t ins = cpu_decode(cpu, insptr);
        printf("$%.4x: ", cpu->frame.pc);
        print_ins(ins);
        cpu_execute(cpu, ins);
    }

    cpu->frame.pc = prev_pc + 1;
    printf("Program halted.\n");

    // Dump state.
    dump_state(cpu);
}

bool init(void) {
    // Initialize SDL.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

    // Create the main window.
	mainWindow = SDL_CreateWindow("NES Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 254, 240, SDL_WINDOW_SHOWN);
	if (mainWindow == NULL) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

    // Get the main window surface.
	mainSurface = SDL_GetWindowSurface(mainWindow);

	// Create the main rendering context.
	mainRenderer = SDL_CreateRenderer(mainWindow, -1, SDL_RENDERER_ACCELERATED);

    // Set the blend mode.
	SDL_SetRenderDrawBlendMode(mainRenderer, SDL_BLENDMODE_BLEND);

	// Clear the rendering surface.
	SDL_SetRenderDrawColor(mainRenderer, 0, 0, 0, 255);
	SDL_RenderClear(mainRenderer);

    // Present the rendering surface.
	SDL_RenderSetScale(mainRenderer, 1, 1);
	SDL_RenderPresent(mainRenderer);

    return true;
}

void exit_handler() {
    // Destroy CPU.
    if (cpu != NULL) {
        cpu_destroy(cpu);
    }

    // Destroy PPU.
    if (ppu != NULL) {
        ppu_destroy(ppu);
    }

    // Destroy the window and renderer.
    if (mainRenderer != NULL)
	    SDL_DestroyRenderer(mainRenderer);
    if (mainWindow != NULL)
	    SDL_DestroyWindow(mainWindow);

    // Quit SDL subsystems.
    SDL_Quit();
}

void keyboard_interupt_handler(int signum) {
    char buffer[50];
    interrupted = true;
    while (interrupted) {
        printf("> ");
        scanf("%49s", buffer);
        if (strcmp(buffer, "reset") == 0) {
            cpu_reset(cpu);
            interrupted = false;
        }
        else if (strcmp(buffer, "quit") == 0) {
            exit(0);
        }
        else if (strcmp(buffer, "history") == 0) {
            print_hist(history, HIST_LEN);
        }
        else if (strcmp(buffer, "state") == 0) {
            dump_state(cpu);
        }
        else if (strcmp(buffer, "continue") == 0) {
            interrupted = false;
        }
        else {
            printf("Invalid command.\n");
        }
    }

    signal(SIGINT, keyboard_interupt_handler);
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

void dump_state(cpu_t *cpu) {
    printf("pc: $%.4x, a: %d. x: %d, y: %d, sp: $%.2x, sr: ", cpu->frame.pc, cpu->frame.ac, cpu->frame.x, cpu->frame.y, cpu->frame.sp);
    printf(cpu->frame.sr.flags.neg ? "n" : "-");
    printf(cpu->frame.sr.flags.vflow ? "v" : "-");
    printf(cpu->frame.sr.flags.ign ? "-" : "-");
    printf(cpu->frame.sr.flags.brk ? "b" : "-");
    printf(cpu->frame.sr.flags.dec ? "d" : "-");
    printf(cpu->frame.sr.flags.irq ? "i" : "-");
    printf(cpu->frame.sr.flags.zero ? "z" : "-");
    printf(cpu->frame.sr.flags.carry ? "c" : "-");
    printf("\n");
}

void print_hist(history_t *hist, int nhist) {
    bool printed = false;
    for (int i = 0; i < HIST_LEN; i++) {
        if (history[i].op.instruction != NULL) {
            printf("$%.4x: ", history[i].pc);
            print_ins(history[i].op);
            printed = true;
        }
    }
    if (!printed) {
        printf("No instructions to display.\n");
    }
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
    else if (ins.addr_mode == &AM_INDIRECT_Y) {
        printf(" ($%.2x),Y", ins.args[0]);
    }
    printf("\n");
}

bool strprefix(const char *str, const char *pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}
