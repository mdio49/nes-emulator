#include <color.h>
#include <ppu.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*static uint8_t get_palette_color(ppu_t *ppu, uint8_t tile_x, uint8_t tile_y, uint8_t fine_x, uint8_t fine_y) {
    addr_t entry = (tile_y << 8) + (tile_x << 4) + fine_y;
    uint8_t plane1 = as_read(ppu->as, entry);
    uint8_t plane2 = as_read(ppu->as, entry + 0x08);
    uint8_t mask = 0x80 >> fine_x;
    return (plane1 & mask) | ((plane2 & mask) << 1);
}*/

/**
 * @brief Gets the address of the corresponding nametable entry.
 * 
 * @param entry The nametable entry.
 * @return The resultant address.
 */
static inline addr_t get_nt_addr(nt_entry_t entry) {
    return (0x02 << 12) | (entry.nt_y << 11) | (entry.nt_x << 10) | (entry.cell_y << 5) | entry.cell_x;
}

/**
 * @brief Gets the address of the corresponding pattern table entry.
 * 
 * @param entry The pattern table entry.
 * @return The resultant address.
 */
static inline addr_t get_pt_addr(pt_entry_t entry) {
    return (entry.table << 9) | (entry.tile_y << 8) | (entry.tile_x << 4) | (entry.plane << 3) | entry.fine_y;
}

/**
 * @brief Gets the attribute table address of the corresponding nametable entry.
 * 
 * @param entry The nametable entry.
 * @return The resultant attribute table address.
 */
static inline addr_t get_at_addr(nt_entry_t entry) {
    return (0x02 << 12) | (entry.nt_y << 11) | (entry.nt_x << 10) | (0x0F << 6) | ((entry.cell_y >> 2) << 3) | (entry.cell_x >> 2);
}

static inline pt_entry_t fetch_tile(ppu_t *ppu, nt_entry_t nt) {
    pt_entry_t result = { 0 };
    addr_t nt_addr = get_nt_addr(nt);
    uint8_t tile = as_read(ppu->as, nt_addr);
    result.tile_x = tile & 0x0F;
    result.tile_y = (tile & 0xF0) >> 4;
    return result;
}

static inline void fetch_tile_planes(ppu_t *ppu, pt_entry_t pt, uint8_t *p1, uint8_t *p2) {
    addr_t pt_addr = get_pt_addr(pt);
    *p1 = as_read(ppu->as, pt_addr);
    *p2 = as_read(ppu->as, pt_addr + 0x08);
}

static inline void put_pixel(ppu_t *ppu, int screen_x, int screen_y, color_t color) {
    int index = (screen_x + screen_y * SCREEN_WIDTH) * 3;
    ppu->out[index + OUT_R] = color.red;
    ppu->out[index + OUT_G] = color.green;
    ppu->out[index + OUT_B] = color.blue;
}

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
    // PPUCONTROL
    if (ppu->ppucontrol_flags.write) {
        ppu->t.nt_x = ppu->controller.nt_addr & 0x01;
        ppu->t.nt_y = (ppu->controller.nt_addr >> 1) & 0x01;
        ppu->ppucontrol_flags.write = 0;
    }

    // PPUSTATUS
    if (ppu->ppustatus_flags.read) {
        ppu->status.vblank = 0;
        ppu->ppustatus_flags.read = 0;
        ppu->w = 0;
    }

    // PPUSCROLL
    if (ppu->ppuscroll_flags.write) {
        if (ppu->w) {
            // second write
            ppu->t.fine_y = ppu->scroll & 0x07;
            ppu->t.coarse_y = ppu->scroll >> 3;
        }
        else {
            // first write
            ppu->x = ppu->scroll & 0x07;
            ppu->t.coarse_x = ppu->scroll >> 3;
        }
        ppu->ppuscroll_flags.write = 0;
        ppu->w= !ppu->w;
    }

    // PPUADDR
    if (ppu->ppuaddr_flags.write) {
        if (ppu->w) {
            // second write
            //ppu->t.coarse_x = ppu->ppu_addr & 0x1F;
            //ppu->t.coarse_y = (ppu->t.coarse_y & ~0x07) | (ppu->ppu_addr >> 5);
            ppu->v = ppu->t;

            ppu->vram_write_addr = (ppu->vram_write_addr & 0xFF00) | ppu->ppu_addr;
        }
        else {
            // first write
            //ppu->v.coarse_y = (ppu->v.coarse_y & 0x07) | ((ppu->ppu_addr & 0x03) << 3);
            //ppu->v.nt_x = (ppu->ppu_addr >> 2) & 0x01;
            //ppu->v.nt_y = (ppu->ppu_addr >> 3) & 0x01;
            //ppu->v.fine_y = (ppu->ppu_addr >> 4) & 0x03;

            ppu->vram_write_addr = (ppu->vram_write_addr & 0x00FF) | (ppu->ppu_addr << 8);
        }
        ppu->ppuaddr_flags.write = 0;
        ppu->w= !ppu->w;
    }

    // PPUDATA
    if (ppu->ppudata_flags.write) {
        as_write(ppu->as, ppu->vram_write_addr, ppu->ppu_data);
        ppu->vram_write_addr += ppu->controller.vram_inc ? 32 : 1;
        ppu->ppudata_flags.write = 0;
    }
    if (ppu->ppudata_flags.read) {
        ppu->ppu_data = as_read(ppu->as, ppu->vram_write_addr);
        ppu->vram_write_addr += ppu->controller.vram_inc ? 32 : 1;
        ppu->ppudata_flags.read = 0;
    }

    clock_t t = clock();
    double dt = (double)(t - ppu->last_time) / CLOCKS_PER_SEC;
    ppu->frame_counter += dt;
    ppu->last_time = t;
    if (ppu->frame_counter >= TIME_STEP) {
        ppu->status.vblank = 1;
        ppu->frame_counter -= TIME_STEP;
        ppu->flush_flag = true;

        const color_t white = { 255, 255, 255 };
        const color_t black = { 0, 0, 0 };

        uint8_t p1, p2;
        nt_entry_t nt;
        pt_entry_t pt;
        
        // For each scanline.
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            nt.cell_x = ppu->v.coarse_x;
            nt.cell_y = ppu->v.coarse_y;

            nt.nt_x = ppu->v.nt_x;
            nt.nt_y = ppu->v.nt_y;

            pt = fetch_tile(ppu, nt);
            pt.fine_x = ppu->x;
            pt.fine_y = ppu->v.fine_y;
            fetch_tile_planes(ppu, pt, &p1, &p2);

            // Move across the scanline.
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                // Determine the value of the bit at this pixel of the tile.
                uint8_t mask = 0x80 >> ppu->x;
                uint8_t b0 = (p1 & mask) > 0;
                uint8_t b1 = (p2 & mask) > 0;
                uint8_t val = (b1 << 1) | b0;

                // Output the pixel (black or white for now).
                put_pixel(ppu, x, y, val > 0 ? white : black);

                // Increment x.
                if (ppu->x < 7) {
                    ppu->x++;
                }
                else {
                    ppu->x = 0;
                    if (ppu->v.coarse_x < 31) {
                        ppu->v.coarse_x++;
                    }
                    else {
                        ppu->v.coarse_x = 0;
                        ppu->v.nt_x = !ppu->v.nt_x;
                    }

                    // Fetch the next tile.
                    nt.cell_x = ppu->v.coarse_x;
                    pt = fetch_tile(ppu, nt);
                    pt.fine_x = ppu->x;
                    pt.fine_y = ppu->v.fine_y;
                    fetch_tile_planes(ppu, pt, &p1, &p2);
                }
            }

            // Reset x.
            ppu->v.coarse_x = ppu->t.coarse_x;
            ppu->v.nt_x = ppu->t.nt_x;

            // Increment y.
            if (ppu->v.fine_y < 7) {
                ppu->v.fine_y++;
            }
            else {
                ppu->v.fine_y = 0;
                if (ppu->v.coarse_y == 29) {
                    ppu->v.coarse_y = 0;
                    ppu->v.nt_y = !ppu->v.nt_y;
                }
                else if (ppu->v.coarse_y == 31) {
                    ppu->v.coarse_y = 0;
                }
                else {
                    ppu->v.coarse_y++;
                }
            }
        }

        // Reset y.
        ppu->v.coarse_y = ppu->t.coarse_y;
        ppu->v.fine_y = ppu->t.fine_y;
        ppu->v.nt_y = ppu->t.nt_y;
    }
}
