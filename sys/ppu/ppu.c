#include <ppu.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*static uint8_t get_palette_color(uint8_t *nametable, uint8_t tile_x, uint8_t tile_y, uint8_t fine_x, uint8_t fine_y) {
    uint8_t *plane1 = nametable + (tile_y << 8) + (tile_x << 4) + fine_y;
    uint8_t *plane2 = plane1 + 0x08;
    uint8_t mask = 0x80 >> fine_x;
    return (*plane1 & mask) | ((*plane2 & mask) << 1);
}*/

ppu_t *ppu_create(void) {
    ppu_t *ppu = malloc(sizeof(struct ppu));
    ppu->vram = malloc(sizeof(uint8_t) * VRAM_SIZE);
    ppu->as = as_create();
    ppu->flush_flag = false;
    ppu->last_time = clock();
    return ppu;
}

void ppu_destroy(ppu_t *ppu) {
    as_destroy(ppu->as);
    free(ppu->vram);
    free(ppu);
}

void ppu_render(ppu_t *ppu, int cycles) {
    // PPUSTATUS
    if (ppu->ppustatus_flags.read) {
        ppu->status.vblank = 0;
        ppu->ppustatus_flags.read = 0;
    }

    // PPUADDR
    if (ppu->ppuaddr_flags.write) {
        if (ppu->ppuaddr_flags.low) {
            ppu->v = (ppu->v & 0xFF00) | ppu->ppu_addr;
        }
        else {
            ppu->v = (ppu->v & 0x00FF) | (ppu->ppu_addr << 8);
        }
        ppu->ppuaddr_flags.write = 0;
        ppu->ppuaddr_flags.low = !ppu->ppuaddr_flags.low;
    }

    // PPUDATA
    if (ppu->ppudata_flags.write) {
        as_write(ppu->as, ppu->v, ppu->ppu_data);
        ppu->v += ppu->controller.vram_inc ? 32 : 1;
        ppu->ppudata_flags.write = 0;
    }
    if (ppu->ppudata_flags.read) {
        ppu->ppu_data = as_read(ppu->as, ppu->v);
        ppu->v += ppu->controller.vram_inc ? 32 : 1;
        ppu->ppudata_flags.read = 0;
    }

    clock_t t = clock();
    clock_t dt = t - ppu->last_time;
    ppu->last_time = t;
    ppu->frame_counter += dt;
    if (ppu->frame_counter >= TIME_STEP) {
        ppu->status.vblank = 1;
        ppu->frame_counter -= TIME_STEP;
        ppu->flush_flag = true;
    }
}
