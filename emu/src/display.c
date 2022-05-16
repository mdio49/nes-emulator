#include <emu.h>

static void poll_events(void);

const char *title = "NES Emulator";

static SDL_Window *window = NULL;
static SDL_Surface *surface = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *screen = NULL;

static uint64_t frame_counter = 0;
static uint64_t last_fps = 0;

static bool fullscreen = false;

bool init_display(void) {
    // Create the main window.
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		printf("Window could not be created: %s\n", SDL_GetError());
		return false;
	}

    // Ensure that the window can't be made smaller than the raw output of the PPU.
    SDL_SetWindowMinimumSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Get the main window surface.
	surface = SDL_GetWindowSurface(window);

	// Create the main rendering context.
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Set the blend mode.
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Create a texture for the screen.
    screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);

    return true;
}

void free_display(void) {
    if (renderer != NULL)
        SDL_DestroyRenderer(renderer);
    if (surface != NULL)
        SDL_FreeSurface(surface);
    if (window != NULL)
	    SDL_DestroyWindow(window);
}

void toggle_fullscreen(void) {
    SDL_SetWindowFullscreen(window, !fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    fullscreen = !fullscreen;
}

bool is_fullscreen(void) {
    return fullscreen;
}

void update_screen(const char *data) {
    // Poll events.
    poll_events();

    // Clear the rendering surface.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    // Get the window size.
    int Sw, Sh;
    SDL_GetWindowSize(window, &Sw, &Sh);

    // Determine the scaling factor.
    const double sx = (double)Sw / SCREEN_WIDTH;
    const double sy = (double)Sh / SCREEN_HEIGHT;
    const double scale = min(sx, sy);

    // Determine the viewport offset.
    const int Vx = (Sw / scale - SCREEN_WIDTH) / 2;
    const int Vy = (Sh / scale - SCREEN_HEIGHT) / 2;

    // Update and copy the texture to the surface.
    SDL_Rect rect = { Vx, Vy, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_UpdateTexture(screen, NULL, data, PIXEL_STRIDE * SCREEN_WIDTH);
    SDL_RenderCopy(renderer, screen, NULL, &rect);

    // Present the rendering surface.
    SDL_RenderSetLogicalSize(renderer, Sw, Sh);
    SDL_RenderSetScale(renderer, scale, scale);
    SDL_RenderPresent(renderer);

    // Keep track of the FPS.
    frame_counter++;
    uint64_t ticks = SDL_GetTicks64();
    uint64_t delta = ticks - last_fps;
    if (delta >= 1000) {
        char window_title[50];
        sprintf(window_title, "%s (FPS: %llu)", title, frame_counter);
        SDL_SetWindowTitle(window, window_title);

        last_fps += 1000;
        frame_counter = 0;
    }
}

static void poll_events(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        switch (e.type) {
            case SDL_QUIT:
                exit(0);
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    exit(0);
                }
                if (e.key.keysym.scancode == SDL_SCANCODE_R) {
                    sys_reset();
                }
                if (e.key.keysym.scancode == SDL_SCANCODE_L) {
                    if (is_logging()) {
                        end_log();
                        printf("Logging stopped.\n");
                    }
                    else {
                        start_log();
                        printf("Logging started.\n");
                    }
                }
                if (e.key.keysym.scancode == SDL_SCANCODE_M) {
                    toggle_audio();
                }
                if (e.key.keysym.scancode == SDL_SCANCODE_F4) {
                    toggle_fullscreen();
                }
                break;
        }
    }
}
