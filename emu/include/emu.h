#include <addrmodes.h>
#include <color.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys.h>

// Include SDL.
#define SDL_MAIN_HANDLED
#include <sdl.h>

#define SCREEN_SCALE    3
#define FRAME_RATE      60
#define TIME_STEP       (1000.0 / FRAME_RATE)

#define HIST_LEN        50

typedef struct history {

    operation_t     op;
    uint16_t        pc;

} history_t;

/* init functions */

bool init(void);
bool init_audio(void);
bool init_display(void);

/* free functions */

void free_display(void);

/* run functions */

void run_bin(const char *path, bool test);
void run_hex(int argc, char *argv[]);

/* callback functions */

void before_execute(operation_t ins);
void after_execute(operation_t ins);

void update_screen(const char *data);

uint8_t poll_input_p1(void);
uint8_t poll_input_p2(void);

/* util functions */

const char *load_rom(const char *path);

void dump_state();

void print_hist();
void print_ins(FILE *fp, operation_t ins);
void print_state(FILE *fp, cpu_t *cpu);

bool strprefix(const char *str, const char *pre);

/* variables */

extern SDL_Window *mainWindow;
extern history_t history[HIST_LEN];
