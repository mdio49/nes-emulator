#include <emu.h>

void exit_handler(void);
void keyboard_interupt_handler(int signum);

SDL_Window *mainWindow = NULL;

history_t history[HIST_LEN] = { 0 };
handlers_t handlers = {
    .interrupted = false
};

int status = 0x00;
int msg_ptr = 0x6004;

bool test = false;
bool no_audio = false;
FILE *log_fp = NULL;

int main(int argc, char *argv[]) {
    // Setup signal handlers.
    signal(SIGINT, keyboard_interupt_handler);
    atexit(exit_handler);

    // Turn on the system.
    sys_poweron();

    // Parse CL arguments.
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
        else if (strcmp(arg, "-l") == 0) {
            log_fp = fopen("emu.log", "w");
        }
        else if (strcmp(arg, "-nogui") == 0) {
            nogui = true;
        }
        else if (strcmp(arg, "-m") == 0) {
            no_audio = true;
        }
        else if (!strprefix(arg, "-") && path == NULL) {
            path = arg;
        }
        else {
            printf("Usage: %s [<path|-x hex...>] [-l] [-m] [-nogui] [-t]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    
    if (path == NULL) {
        printf("Usage: %s [<path|-x hex...>] [-l] [-m] [-nogui] [-t]\n", argv[0]);
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
	mainWindow = SDL_CreateWindow("NES Emulator (FPS: 0)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * SCREEN_SCALE, SCREEN_HEIGHT * SCREEN_SCALE, SDL_WINDOW_SHOWN);
	if (mainWindow == NULL) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

    // Initialize subsystems.
    if (!no_audio && !init_audio()) {
        return false;
    }
    if (!init_display()) {
        return false;
    }

    return true;
}

void exit_handler() {
    // Close log file pointer (if it exists).
    if (log_fp != NULL) {
        fclose(log_fp);
    }

    // Turn off the system.
    sys_poweroff();

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

    // Attach the program to the system.
    sys_insert(prog);

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
    as_add_segment(as, 0, 65536, mem);
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

    // Write to the log.
    if (log_fp != NULL) {
        fprintf(log_fp, "$%.4x:", cpu->frame.pc);
        fprintf(log_fp, " %.2X", ins.opc);
        if (ins.addr_mode->argc > 0)
            fprintf(log_fp, " %.2X", ins.args[0]);
        else
            fprintf(log_fp, "   ");
        if (ins.addr_mode->argc > 1)
            fprintf(log_fp, " %.2X", ins.args[1]);
        else
            fprintf(log_fp, "   ");
        fprintf(log_fp, " \t|\t");
        print_ins(log_fp, ins);
        fprintf(log_fp, "\t|\t");
        print_state(log_fp, cpu);
        fprintf(log_fp, "\t|\tPPU: %3d, %3d CPU: %lld\n", ppu->draw_x, ppu->draw_y, cpu->cycles);
    }
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
        result |= 0x01;
    if (keystate[SDL_SCANCODE_LCTRL])
        result |= 0x02;
    if (keystate[SDL_SCANCODE_RSHIFT])
        result |= 0x04;
    if (keystate[SDL_SCANCODE_RETURN])
        result |= 0x08;
    if (keystate[SDL_SCANCODE_UP])
        result |= 0x10;
    if (keystate[SDL_SCANCODE_DOWN])
        result |= 0x20;
    if (keystate[SDL_SCANCODE_LEFT])
        result |= 0x40;
    if (keystate[SDL_SCANCODE_RIGHT])
        result |= 0x80;
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
