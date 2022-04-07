#include <ppu.h>
#include <stdlib.h>

static uint8_t get_palette_color(uint8_t *nametable, uint8_t tile_x, uint8_t tile_y, uint8_t fine_x, uint8_t fine_y) {
    uint8_t *plane1 = nametable + (tile_y << 8) + (tile_x << 4) + fine_y;
    uint8_t *plane2 = plane1 + 0x08;
    uint8_t mask = 0x80 >> fine_x;
    return (*plane1 & mask) | ((*plane2 & mask) << 1);
}

ppu_t *ppu_create(void) {
    ppu_t *ppu = malloc(sizeof(struct ppu));
    ppu->vram = malloc(sizeof(uint8_t) * VRAM_SIZE);
    ppu->as = as_create();
    return ppu;
}

void ppu_destroy(ppu_t *ppu) {
    as_destroy(ppu->as);
    free(ppu->vram);
    free(ppu);
}
