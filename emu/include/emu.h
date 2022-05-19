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

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

#define WINDOW_WIDTH    (SCREEN_WIDTH * 3)
#define WINDOW_HEIGHT   (SCREEN_HEIGHT * 3)

/* init functions */

bool init(void);
bool init_audio(void);
bool init_display(void);

/* free functions */

void free_audio(void);
void free_display(void);

/* emulator functions */

void exit_handler(void);

void run_bin(const char *path, bool test);
void run_hex(int argc, char *argv[]);

void pause(void);
void resume(void);
bool is_paused(void);

/* callback functions */

void before_execute(operation_t ins);
void after_execute(operation_t ins);

void update_screen(const char *data);

uint8_t poll_input_p1(void);
uint8_t poll_input_p2(void);

/* log functions */

void start_log(void);
void end_log(void);
void log_ins(operation_t ins);
bool is_logging(void);

/* audio functions */

void toggle_audio(void);
bool is_muted(void);

/* display functions */

void toggle_fullscreen(void);
bool is_fullscreen(void);

/* util functions */

const char *load_rom(const char *path);
const char *load_save(const char *path);

void dump_state(cpu_t *cpu);

void print_ins(FILE *fp, operation_t ins);
void print_state(FILE *fp, cpu_t *cpu);

bool strprefix(const char *str, const char *pre);
