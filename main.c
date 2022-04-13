#include <addrmodes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys.h>

// Include SDL.
#define SDL_MAIN_HANDLED
#include <sdl.h>

#define SCREEN_SCALE 3

#define HIST_LEN 50

typedef struct history {

    operation_t     op;
    uint16_t        pc;

} history_t;

bool init();
void exit_handler(void);
void keyboard_interupt_handler(int signum);

void run_bin(const char *path, bool test);
void run_hex(int argc, char *argv[]);

void test_mode_before_execute(operation_t ins);
void test_mode_after_execute(operation_t ins);

const char *load_rom(const char *path);

void dump_state();
void print_hist();
void print_ins(operation_t ins);

bool strprefix(const char *str, const char *pre);

SDL_Window *mainWindow = NULL;
SDL_Surface *mainSurface = NULL;
SDL_Renderer *mainRenderer = NULL;
SDL_Texture *screen = NULL;

history_t history[HIST_LEN] = { 0 };
handlers_t handlers = { .interrupted = false };

int status = 0x00;
int msg_ptr = 0x6004;

char pixels[SCREEN_WIDTH * SCREEN_HEIGHT * 3] = { 0 };

int main(int argc, char *argv[]) {
    // Setup signal handlers.
    signal(SIGINT, keyboard_interupt_handler);
    atexit(exit_handler);

    // Turn on the system.
    sys_poweron();

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

bool init(void) {
    // Initialize SDL.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

    // Create the main window.
	mainWindow = SDL_CreateWindow("NES Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * SCREEN_SCALE, SCREEN_HEIGHT * SCREEN_SCALE, SDL_WINDOW_SHOWN);
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

    // Create a texture for the screen.
    screen = SDL_CreateTexture(mainRenderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);

    pixels[0] = 255;
    pixels[3] = 255;
    pixels[6] = 255;
    pixels[9] = 255;

    return true;
}

void exit_handler() {
    // Turn off the system.
    sys_poweroff();

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
    handlers.interrupted = true;
    while (handlers.interrupted) {
        printf("> ");
        scanf("%49s", buffer);
        if (strcmp(buffer, "reset") == 0) {
            sys_reset();
            handlers.interrupted = false;
        }
        else if (strcmp(buffer, "quit") == 0) {
            handlers.running = false;
            handlers.interrupted = false;
        }
        else if (strcmp(buffer, "history") == 0) {
            print_hist(history, HIST_LEN);
        }
        else if (strcmp(buffer, "state") == 0) {
            dump_state(cpu);
        }
        else if (strcmp(buffer, "continue") == 0) {
            handlers.interrupted = false;
        }
        else {
            printf("Invalid command.\n");
        }
    }

    signal(SIGINT, keyboard_interupt_handler);
}

void run_bin(const char *path, bool test) {
    // Load the program.
    const char *src = load_rom(path);
    prog_t *prog = prog_create(src);
    if (prog == NULL) {
        fprintf(stderr, "Unable to load ROM.");
        exit(1);
    }

    // Attach the program to the system.
    sys_insert(prog);

    // Setup the handlers.
    handlers.before_execute = test ? test_mode_before_execute : NULL;
    handlers.after_execute = test ? test_mode_after_execute : NULL;

    // Run the system.
    sys_run(&handlers);
}

void run_hex(int argc, char *bytes[]) {
    // Starting address; consistent with easy 6502.
    const addr_t start = 0x0600;

    // Setup a simple address space that uses a single 64KB segment.
    uint8_t mem[65536];
    addrspace_t *as = as_create();
    as_add_segment(as, 0, 65536, mem);
    as_destroy(cpu->as);
    cpu->as = as;

    // Load program from input.
    for (int i = 0; i < argc; i++) {
        as_write(cpu->as, start + i, strtol(bytes[i], NULL, 16));
    }

    // Execute program.
    addr_t prev_pc = 0;
    cpu->frame.pc = start;
    while (cpu->frame.sr.brk == 0) {
        prev_pc = cpu->frame.pc;
        uint8_t opc = cpu_fetch(cpu);
        operation_t ins = cpu_decode(cpu, opc);
        printf("$%.4x: ", cpu->frame.pc);
        print_ins(ins);
        cpu_execute(cpu, ins);
    }

    cpu->frame.pc = prev_pc + 1;
    printf("Program halted.\n");

    // Dump state.
    dump_state(cpu);
}

void test_mode_before_execute(operation_t ins) {
    // Update history.
    for (int i = 0; i < HIST_LEN - 1; i++) {
        history[i] = history[i + 1];
    }
    history[HIST_LEN - 1].pc = cpu->frame.pc;
    history[HIST_LEN - 1].op = ins;
}

void test_mode_after_execute(operation_t ins) {
    ppu_flush(pixels);

    // Display a message if available.
    char msg = as_read(cpu->as, msg_ptr);
    if (msg != '\0') {
        putchar(msg);
        msg_ptr++;
    }

    // Update status of test.
    int new_status = as_read(cpu->as, 0x6000);
    if (new_status != status) {
        switch (new_status) {
            case 0x80:
                printf("Test running...\n");
                break;
            case 0x81:
                printf("Reset required.\n");
                break;
            default:
                printf("Test completed with result code %d.\n", new_status);
                handlers.running = false;
                break;
        }
        status = new_status;
    }
}

void ppu_flush(const char *data) {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        switch (e.type) {
            case SDL_QUIT:
                exit(0);
                break;
        }
    }

    // Clear the rendering surface.
    SDL_SetRenderDrawColor(mainRenderer, 0, 0, 0, 0);
    SDL_RenderClear(mainRenderer);

    // Update and copy the texture to the surface.
    SDL_Rect rect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_UpdateTexture(screen, NULL, data, 3 * SCREEN_WIDTH);
    SDL_RenderCopy(mainRenderer, screen, NULL, &rect);

    // Present the rendering surface.
    SDL_RenderSetScale(mainRenderer, SCREEN_SCALE, SCREEN_SCALE);
    SDL_RenderPresent(mainRenderer);
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
    printf(cpu->frame.sr.neg ? "n" : "-");
    printf(cpu->frame.sr.vflow ? "v" : "-");
    printf(cpu->frame.sr.ign ? "-" : "-");
    printf(cpu->frame.sr.brk ? "b" : "-");
    printf(cpu->frame.sr.dec ? "d" : "-");
    printf(cpu->frame.sr.irq ? "i" : "-");
    printf(cpu->frame.sr.zero ? "z" : "-");
    printf(cpu->frame.sr.carry ? "c" : "-");
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
