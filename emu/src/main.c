#include <emu.h>

void exit_handler(void);
void keyboard_interupt_handler(int signum);

static char *get_sav_path(const char *rom_path);

SDL_Window *mainWindow = NULL;

history_t history[HIST_LEN] = { 0 };
handlers_t handlers = {
    .interrupted = false
};

char *sav_path = NULL;
size_t sav_data_size;

int status = 0x00;
int msg_ptr = 0x6004;

bool test = false;

int main(int argc, char *argv[]) {
    // Setup signal handlers.
    signal(SIGINT, keyboard_interupt_handler);
    atexit(exit_handler);

    // Turn on the system.
    sys_poweron();

    // Parse CL arguments.
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
        else if (strcmp(arg, "-l") == 0) {
            start_log();
        }
        else if (!strprefix(arg, "-") && path == NULL) {
            path = arg;
        }
        else {
            printf("Usage: %s [<path|-x hex...>] [-l] [-t]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    
    if (path == NULL) {
        printf("Usage: %s [<path|-x hex...>] [-l] [-t]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!init()) {
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
	mainWindow = SDL_CreateWindow("NES Emulator (FPS: 0)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (mainWindow == NULL) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

    // Ensure that the window can't be made smaller than the raw output of the PPU.
    SDL_SetWindowMinimumSize(mainWindow, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Initialize subsystems.
    if (!init_audio()) {
        return false;
    }
    if (!init_display()) {
        return false;
    }

    return true;
}

void exit_handler() {
    // Close log (if it's open).
    end_log();

    // Save PRG-RAM data.
    if (sav_path != NULL) {
        FILE *fp = fopen(sav_path, "wb");
        fwrite(curprog->prg_ram, sizeof(char), sav_data_size, fp);
        fclose(fp);
        free(sav_path);
    }

    // Turn off the system.
    sys_poweroff();

    // Free the audio.
    free_audio();

    // Free the display.
    free_display();

    // Destroy the window.
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

    // Set to PAL if there is an '(E)' in the filename.
    if (strstr(path, "(E)")) {
        tv_sys = TV_SYS_PAL;
    }

    // Attach the program to the system.
    sys_insert(prog);

    // Load save data.
    if (prog->header.prg_ram) {
        // Determine size of save data.
        sav_data_size = prog->header.prg_ram_size * INES_PGR_RAM_UNIT;
        if (sav_data_size == 0) {
            sav_data_size = INES_PGR_RAM_UNIT; // Value of 0 in header infers 8KB for compatibility.
        }

        // Read the save data into PRG-RAM.
        sav_path = get_sav_path(path);
        const char *sav = load_save(sav_path);
        if (sav != NULL) {
            memcpy(prog->prg_ram, sav, sav_data_size);
            free((void*)sav); // Free the save data buffer.
        }
    }

    // Setup the handlers.
    handlers.before_execute = before_execute;
    handlers.after_execute = after_execute;
    handlers.update_screen = update_screen;
    handlers.poll_input_p1 = poll_input_p1;
    handlers.poll_input_p2 = poll_input_p2;

    // Run the system.
    sys_run(&handlers);
}

void run_hex(int argc, char *bytes[]) {
    // Starting address; consistent with easy 6502.
    const addr_t start = 0x0600;

    // Setup a simple address space that uses a single 64KB segment.
    uint8_t mem[65536];
    addrspace_t *as = as_create();
    as_add_segment(as, 0, 65536, mem, AS_READ | AS_WRITE);
    as_destroy(cpu->as);
    cpu->as = as;

    // Load program from input.
    for (int i = 0; i < argc; i++) {
        as_write(cpu->as, start + i, strtol(bytes[i], NULL, 16));
    }

    // Execute program.
    uint8_t opc = 0x01;
    addr_t prev_pc = 0;
    cpu->frame.pc = start;
    while (opc != 0x00) {
        prev_pc = cpu->frame.pc;
        opc = cpu_fetch(cpu);
        operation_t ins = cpu_decode(cpu, opc);
        printf("$%.4x: ", cpu->frame.pc);
        print_ins(stdout, ins);
        printf("\n");
        cpu_execute(cpu, ins);
    }

    cpu->frame.pc = prev_pc + 1;
    printf("Program halted.\n");

    // Dump state.
    dump_state(cpu);
}

void before_execute(operation_t ins) {
    // Update history.
    for (int i = 0; i < HIST_LEN - 1; i++) {
        history[i] = history[i + 1];
    }
    history[HIST_LEN - 1].pc = cpu->frame.pc;
    history[HIST_LEN - 1].op = ins;

    // Log the instruction.
    log_ins(ins);
}

void after_execute(operation_t ins) {
    if (test) {
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
}

uint8_t poll_input_p1(void) {
    const uint8_t *keystate = SDL_GetKeyboardState(NULL);
    uint8_t result = 0;
    if (keystate[SDL_SCANCODE_SPACE])
        result |= JOYPAD_A;
    if (keystate[SDL_SCANCODE_LCTRL])
        result |= JOYPAD_B;
    if (keystate[SDL_SCANCODE_RSHIFT])
        result |= JOYPAD_SELECT;
    if (keystate[SDL_SCANCODE_RETURN])
        result |= JOYPAD_START;
    if (keystate[SDL_SCANCODE_UP])
        result |= JOYPAD_UP;
    else if (keystate[SDL_SCANCODE_DOWN])
        result |= JOYPAD_DOWN;
    if (keystate[SDL_SCANCODE_LEFT])
        result |= JOYPAD_LEFT;
    else if (keystate[SDL_SCANCODE_RIGHT])
        result |= JOYPAD_RIGHT;
    return result;
}

uint8_t poll_input_p2(void) {
    return 0; // TODO
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

const char *load_save(const char *path) {
    // Open the save data.
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
        return NULL; // If the file couldn't be opened, then return NULL.

    // Get the length of the file.
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    // Allocate memory for the buffer and read in the data.
    char *sav = malloc(size);
    fread(sav, size, 1, fp);
    fclose(fp);

    return sav;
}

static char *get_sav_path(const char *rom_path) {
    // Allocate memory for save path.
    char *sav_path = malloc(strlen(rom_path) + 5);
    strcpy(sav_path, rom_path);

    // Replace the filename extension with '.sav'.
    char *end = sav_path + strlen(rom_path);
    char *p = end;
    while (p > sav_path && *p != '.') {
        p--;
    }
    if (p == sav_path && *p != '.') {
        p = end;
    }
    *p = '\0';
    strcat(p, ".sav");
    
    // Return the save path.
    return sav_path;
}
