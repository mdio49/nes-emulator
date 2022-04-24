#include <color.h>
#include <ppu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    return (entry.table << 12) | (entry.tile_y << 8) | (entry.tile_x << 4) | (entry.plane << 3) | entry.fine_y;
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

static inline void fetch_tile_into_sr(ppu_t *ppu) {
    const nt_entry_t nt = {
        .cell_x = ppu->v.coarse_x,
        .cell_y = ppu->v.coarse_y,
        .nt_x = ppu->v.nt_x,
        .nt_y = ppu->v.nt_y
    };
    
    // Fetch NT byte.
    pt_entry_t pt = fetch_tile(ppu, nt);
    pt.table = ppu->controller.bpt_addr;
    pt.fine_x = ppu->x;
    pt.fine_y = ppu->v.fine_y;

    // Fetch AT byte.
    addr_t attr_addr = get_at_addr(nt);
    ppu->sr8[0] = as_read(ppu->as, attr_addr);

    // Fetch low and high BG tile byte.
    uint8_t p1, p2;
    fetch_tile_planes(ppu, pt, &p1, &p2);
    ppu->sr16[0] = p1 | (p2 << 8);
}

static inline uint8_t get_tile_value(uint8_t p1, uint8_t p2, uint8_t fine_x) {
    uint8_t mask = 0x80 >> fine_x;
    uint8_t b0 = (p1 & mask) > 0;
    uint8_t b1 = (p2 & mask) > 0;
    return (b1 << 1) | b0;
}

static inline void put_pixel(ppu_t *ppu, int screen_x, int screen_y, color_t color) {
    int index = (screen_x + screen_y * SCREEN_WIDTH) * 3;
    ppu->out[index + OUT_R] = color.red;
    ppu->out[index + OUT_G] = color.green;
    ppu->out[index + OUT_B] = color.blue;
}

static inline bool sprite_in_range(ppu_t *ppu, uint8_t sprite_y) {
    return ppu->draw_y >= sprite_y && ppu->draw_y < sprite_y + (ppu->controller.spr_size ? 16 : 8);
}

ppu_t *ppu_create(void) {
    ppu_t *ppu = malloc(sizeof(struct ppu));
    ppu->vram = malloc(sizeof(uint8_t) * VRAM_SIZE);
    ppu->as = as_create();
    ppu->bkg_color = 0x0F;

    ppu->draw_x = 0;
    ppu->draw_y = 0;

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
        ppu->nmi_occurred = false;
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

    // OAMDATA
    if (ppu->oamdata_flags.write) {
        ppu->oam[ppu->oam_addr] = ppu->oam_data;
        ppu->oam_addr++;
        ppu->oamdata_flags.write = 0;
    }
    if (ppu->oamdata_flags.read) {
        ppu->oamdata_flags.read = 0;
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
        ppu->w = !ppu->w;
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

    // Rendering.
    //printf("PPU: %d, %d\n", ppu->draw_x, ppu->draw_y);
    while (cycles > 0) {
        // Render background.
        if (ppu->draw_y == -1) {
            //printf("%d %d\n", ppu->t.coarse_y, ppu->t.nt_y);
            // Pre-render scanline.
            if (ppu->draw_x == 1) {
                ppu->status.vblank = 0;
                ppu->status.overflow = 0;
                ppu->status.hit = 0;
                ppu->nmi_occurred = false;
            }
            else if (ppu->draw_x >= 280 && ppu->draw_x <= 304) {
                // Reset y.
                ppu->v.coarse_y = ppu->t.coarse_y;
                ppu->v.fine_y = ppu->t.fine_y;
                ppu->v.nt_y = ppu->t.nt_y;
            }
            else if (ppu->draw_x == 340) {
                fetch_tile_into_sr(ppu);
            }
        }
        else if (ppu->draw_y == 240) {
            // Post-render scanline.
        }
        else if (ppu->draw_y == 241) {
            // Vblank.
            if (ppu->draw_x == 1) {
                ppu->status.vblank = 1;
                ppu->nmi_occurred = true;
            }
        }
        else if (ppu->draw_y < 240) {
            // Normal rendering.
            if (ppu->draw_x == 0) {
                // Idle
            }
            else if (ppu->draw_x <= 256) {
                // Get the x-coordinate of the pixel on the screen.
                const uint8_t screen_x = ppu->draw_x - 1;

                // Determine the value of the bit at this pixel of the tile.
                uint8_t bkg = get_tile_value(ppu->sr16[0] & 0xFF, ppu->sr16[0] >> 8, ppu->x);
                
                // Determine the color of the pixel.
                uint8_t index = ppu->sr8[0];
                if ((ppu->v.coarse_x & 0x02) > 0)
                    index >>= 2;
                if ((ppu->v.coarse_y & 0x02) > 0)
                    index >>= 4;
                uint8_t col_index = bkg > 0 ? ppu->bkg_palette[(index & 0x03) * 4 + bkg - 1] : ppu->bkg_color;

                // Get active sprites.
                for (int i = 0; i < 8; i++) {
                    if (ppu->oam_x[i] == 0xFF)
                        continue;
                    if (ppu->oam_x[i] > screen_x)
                        continue;
                    if (screen_x > ppu->oam_x[i] + 8)
                        continue;
                    
                    // Get fine-x and flip horizontally if necessary.
                    uint8_t fine_x = screen_x - ppu->oam_x[i];
                    if (ppu->oam_attr[i].flip_h) {
                        fine_x = 7 - fine_x;
                    }

                    // Get value of sprite at current pixel and decide whether it should override the background.
                    uint8_t spr = get_tile_value(ppu->oam_p[i][0], ppu->oam_p[i][1], fine_x);
                    if (spr == 0)
                        continue;
                    if (bkg > 0 && ppu->oam_attr[i].priority)
                        continue;
                    
                    // Update the color and break as any subsequent sprites would be displayed behind this sprite.
                    col_index = ppu->spr_palette[ppu->oam_attr[i].palette * 3 + spr - 1];
                    break;
                }

                // Output the pixel.
                color_t col = color_resolve(col_index);
                put_pixel(ppu, screen_x, ppu->draw_y, col);

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
                    fetch_tile_into_sr(ppu);
                }

                if (ppu->draw_x == 256) {
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
            }
            else if (ppu->draw_x == 257) {
                // Reset x.
                ppu->v.coarse_x = ppu->t.coarse_x;
                ppu->v.nt_x = ppu->t.nt_x;
            }
            else if (ppu->draw_x == 340) {
                fetch_tile_into_sr(ppu);
            }

            // Reset OAMADDR.
            if (ppu->draw_x >= 257 && ppu->draw_x <= 320) {
                ppu->oam_addr = 0;
            }
        }

        // Sprite evaluation.
        if (ppu->draw_y == 0) {
            if (ppu->draw_x >= 257 && ppu->draw_x <= 320) {
                // Reset OAMADDR.
                ppu->oam_addr = 0;
            }
        }
        if (ppu->draw_y < 240) {
            if (ppu->draw_x == 0) {
                // Idle (reset iterators).
                ppu->n = 0;
                ppu->m = 0;
                ppu->oam2_ptr = 0;
            }
            else if (ppu->draw_x <= 64) {
                // Secondary OAM clear.
                ppu->oam2[((ppu->draw_x - 1) >> 1) & 0x1F] = 0xFF;
            }
            else if (ppu->draw_x <= 256) {
                if (ppu->draw_x % 2 == 1) {
                    // Read OAM data on odd cycles into a buffer.
                    uint8_t oam_index = (4 * ppu->n + ppu->m + ppu->oam_addr) & 0xFF;
                    ppu->oam_buffer = ppu->oam[oam_index];
                }
                else if (ppu->oam2_ptr % 4 != 0) {
                    // Copy remaining sprite data into secondary OAM.
                    if (ppu->oam2_ptr < sizeof(ppu->oam2)) {
                        ppu->oam2[ppu->oam2_ptr] = ppu->oam_buffer;
                    }
                    ppu->oam2_ptr++;
                    if (ppu->m == 3) {
                        ppu->n++;
                    }
                    ppu->m++;
                }
                else if (ppu->n >= N_SPRITES) {
                    // All 64 sprites have already been evaluated.
                    if (ppu->oam2_ptr < sizeof(ppu->oam2)) {
                        ppu->oam2[ppu->oam2_ptr] = ppu->oam_buffer;
                    }
                    ppu->n++;
                }
                else if (ppu->oam2_ptr >= sizeof(ppu->oam2)) {
                    // Sprite overflow.
                    if (sprite_in_range(ppu, ppu->oam_buffer)) {
                        ppu->status.overflow = true;
                        ppu->oam2_ptr++;
                        ppu->m++;
                    }
                    else {
                        ppu->n++;
                        ppu->m++; // Sprite overflow bug.
                    }
                }
                else {
                    // Fetch next sprite and check if y-coordinate is within range.
                    ppu->oam2[ppu->oam2_ptr] = ppu->oam_buffer;
                    if (sprite_in_range(ppu, ppu->oam_buffer)) {
                        ppu->oam2_ptr++;
                        ppu->m++;
                    }
                    else {
                        ppu->n++;
                    }
                }
            }
            else if (ppu->draw_x <= 320) {
                // Reset OAMADDR.
                ppu->oam_addr = 0;
            }
            else if (ppu->draw_x == 321) {
                // Fill shift registers.
                for (int i = 0; i < 8; i++) {
                    uint8_t sprite_y = ppu->oam2[4 * i];
                    uint8_t fine_y = ppu->draw_y - sprite_y;
                    uint8_t tile = ppu->oam2[4 * i + 1];

                    // Fetch attribute data.
                    uint8_t attr = ppu->oam2[4 * i + 2];
                    ppu->oam_attr[i].palette = attr & 0x03;
                    ppu->oam_attr[i].priority = (attr >> 5) & 0x01;
                    ppu->oam_attr[i].flip_h = (attr >> 6) & 0x01;
                    ppu->oam_attr[i].flip_v = (attr >> 7) & 0x01;

                    // Flip vertically if necessary.
                    if (ppu->oam_attr[i].flip_v) {
                        fine_y = (ppu->controller.spr_size ? 15 : 7) - fine_y;
                    }

                    // Get pattern table address.
                    addr_t pt_addr;
                    if (ppu->controller.spr_size) {
                        pt_addr = ((tile & 0x01) << 12) | ((tile & ~0x01) << 4) | ((fine_y & 0x08) << 1) | (fine_y & 0x07); // 8x16 sprite mode.
                    }
                    else {
                        pt_addr = (ppu->controller.spt_addr << 12) | (tile << 4) | (fine_y & 0x07); // 8x8 sprite mode.
                    }

                    // Fetch tile planes.
                    ppu->oam_p[i][0] = as_read(ppu->as, pt_addr);
                    ppu->oam_p[i][1] = as_read(ppu->as, pt_addr + 0x08);

                    // Fetch x-position.
                    ppu->oam_x[i] = ppu->oam2[4 * i + 3];
                }
            }
        }

        // Increment scanline pointer.
        ppu->draw_x++;
        if (ppu->draw_x == 341) {
            ppu->draw_x = 0;
            ppu->draw_y++;
        }
        if (ppu->draw_y == 261) {
            ppu->draw_y = -1;
        }

        cycles--;
    }
}
