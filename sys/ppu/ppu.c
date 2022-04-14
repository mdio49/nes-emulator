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
        ppu->t = (ppu->t & ~0x0C00) | (ppu->controller.nt_addr << 10);
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
            ppu->x = ppu->scroll & 0x07;
            ppu->t = (ppu->t & ~0x001F) | (ppu->scroll >> 3);
        }
        else {
            // first write
            ppu->v = (ppu->v & 0x00FF) | (ppu->ppu_addr << 8);
        }
        ppu->ppuscroll_flags.write = 0;
        ppu->w= !ppu->w;
    }

    // PPUADDR
    if (ppu->ppuaddr_flags.write) {
        if (ppu->w) {
            // second write
            ppu->t = (ppu->t & 0xFF00) | ppu->ppu_addr;
            ppu->v = ppu->t;
        }
        else {
            // first write
            ppu->t = (ppu->t & 0x00FF) | ((ppu->ppu_addr & 0x3F) << 8);
        }
        ppu->ppuaddr_flags.write = 0;
        ppu->w= !ppu->w;
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
    double dt = (double)(t - ppu->last_time) / CLOCKS_PER_SEC;
    ppu->frame_counter += dt;
    ppu->last_time = t;
    if (ppu->frame_counter >= TIME_STEP) {
        ppu->status.vblank = 1;
        ppu->frame_counter -= TIME_STEP;
        ppu->flush_flag = true;

        /*uint8_t scroll_x, scroll_y;

        const color_t white = { 255, 255, 255 };
        const color_t black = { 0, 0, 0 };

        nt_entry_t nt;
        pt_entry_t pt;
        for (int y = 0; y < 240; y++) {
            uint8_t abs_y = (y + scroll_y) >> 3;
            nt.cell_y = abs_y;
            nt.nt_y = 

            pt.fine_y = abs_y & 0x07;
            for (nt.cell_x = 0; nt.cell_x < 32; nt.cell_x++) {
                uint8_t x0 = nt.cell_x * 8;
                pt.fine_x = x0 & 0x07;

                // Get the nametable address and the corresponding tile at that nametable entry.
                addr_t nt_addr = get_nt_addr(nt);
                uint8_t tile = as_read(ppu->as, nt_addr);
                pt.tile_x = tile & 0x0F;
                pt.tile_y = (tile & 0xF0) >> 4;

                // Get the pattern table address from the tile and the resultant bytes from both planes.
                addr_t pt_addr = get_pt_addr(pt);
                uint8_t p1 = as_read(ppu->as, pt_addr);
                uint8_t p2 = as_read(ppu->as, pt_addr + 0x08);

                // Draw each pixel from the plane.
                for (pt.fine_x = 0; pt.fine_x < 8; pt.fine_x++) {
                    uint8_t mask = 0x80 >> pt.fine_x;
                    uint8_t b0 = (p1 & mask) > 0;
                    uint8_t b1 = (p2 & mask) > 0;
                    uint8_t val = (b1 << 1) | b0;

                    uint8_t screen_x = x0 + pt.fine_x;
                    uint8_t screen_y = y;

                    put_pixel(ppu, screen_x, screen_y, val > 0 ? white : black);
                }
            }
        }*/

        for (int y = 0; y < NT_ROWS; y++) {
            for (int x = 0; x < NT_COLS; x++) {
                uint8_t nt_entry = x + y * NT_COLS;
                uint8_t pt_entry = as_read(ppu->as, 0x2000 + nt_entry);
                for (int fy = 0; fy < 8; fy++) {
                    addr_t addr = (pt_entry << 4) + fy;
                    uint8_t p1 = as_read(ppu->as, addr);
                    uint8_t p2 = as_read(ppu->as, addr + 0x08);
                    for (int fx = 0; fx < 8; fx++) {
                        uint8_t mask = 0x80 >> fx;
                        uint8_t b0 = (p1 & mask) > 0;
                        uint8_t b1 = (p2 & mask) > 0;
                        uint8_t val = (b1 << 1) | b0;

                        uint8_t screen_x = x * 8 + fx;
                        uint8_t screen_y = y * 8 + fy;

                        int index = (screen_x + screen_y * SCREEN_WIDTH) * 3;
                        if (val > 0) {
                            ppu->out[index + OUT_R] = 0xFF;
                            ppu->out[index + OUT_G] = 0xFF;
                            ppu->out[index + OUT_B] = 0xFF;
                        }
                        else {
                            ppu->out[index + OUT_R] = 0x00;
                            ppu->out[index + OUT_G] = 0x00;
                            ppu->out[index + OUT_B] = 0x00;
                        }
                    }
                }
            }
        }
    }
}
