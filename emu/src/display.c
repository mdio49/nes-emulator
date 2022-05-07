#include <emu.h>

static SDL_Surface *mainSurface = NULL;
static SDL_Renderer *mainRenderer = NULL;
static SDL_Texture *screen = NULL;

static uint64_t last_update = 0;
static uint64_t frame_counter = 0;
static uint64_t last_fps = 0;

bool init_display(void) {
    // Get the main window surface.
	mainSurface = SDL_GetWindowSurface(mainWindow);

	// Create the main rendering context.
	mainRenderer = SDL_CreateRenderer(mainWindow, -1, SDL_RENDERER_ACCELERATED);

    // Set the blend mode.
	SDL_SetRenderDrawBlendMode(mainRenderer, SDL_BLENDMODE_BLEND);

    // Create a texture for the screen.
    screen = SDL_CreateTexture(mainRenderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);

    return true;
}

void free_display(void) {
    if (mainRenderer != NULL)
        SDL_DestroyRenderer(mainRenderer);
    if (mainSurface != NULL)
        SDL_FreeSurface(mainSurface);
}

void update_screen(const char *data) {
    // Poll events.
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

    // Add a delay for the next frame.
    uint64_t delta = 0;
    while (delta < TIME_STEP) {
        uint64_t ticks = SDL_GetTicks64();
        delta += ticks - last_update;
        last_update = ticks;
    }

    // Keep track of the FPS.
    frame_counter++;
    uint64_t ticks = SDL_GetTicks64();
    delta = ticks - last_fps;
    if (delta >= 1000) {
        char title[50];
        sprintf(title, "NES Emulator (FPS: %lld)", frame_counter);
        SDL_SetWindowTitle(mainWindow, title);

        last_fps += 1000;
        frame_counter = 0;
    }
}
