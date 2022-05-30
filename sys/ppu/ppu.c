#include <color.h>
#include <ppu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void render_cycle(ppu_t *ppu, bool rendering, bool vbl_suppress);
static inline void sprite_evaluation(ppu_t *ppu);

/**
 * @brief Gets the address of the corresponding nametable entry.
 * 
 * @param addr The VRAM address.
 * @return The resultant address.
 */
static inline addr_t get_nt_addr(vram_reg_t addr) {
    return 0x2000 | (addr.nt_y << 11) | (addr.nt_x << 10) | (addr.coarse_y << 5) | addr.coarse_x;
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
 * @param addr The VRAM address.
 * @return The resultant attribute table address.
 */
static inline addr_t get_at_addr(vram_reg_t addr) {
    return 0x2000 | (addr.nt_y << 11) | (addr.nt_x << 10) | (0x0F << 6) | ((addr.coarse_y >> 2) << 3) | (addr.coarse_x >> 2);
}

static inline pt_entry_t fetch_tile(ppu_t *ppu) {
    pt_entry_t result = {
        .table = ppu->controller.bpt_addr,
        .fine_y = ppu->v.fine_y
    };
    addr_t nt_addr = get_nt_addr(ppu->v);
    uint8_t tile = as_read(ppu->as, nt_addr);
    result.tile_x = tile & 0x0F;
    result.tile_y = (tile & 0xF0) >> 4;
    return result;
}

static inline void put_pixel(ppu_t *ppu, int screen_x, int screen_y, color_t color) {
    int index = (screen_x + screen_y * SCREEN_WIDTH) * PIXEL_STRIDE;
    ppu->out[index + OUT_R] = color.red;
    ppu->out[index + OUT_G] = color.green;
    ppu->out[index + OUT_B] = color.blue;
}

static inline bool sprite_in_range(ppu_t *ppu, uint8_t sprite_y) {
    return ppu->draw_y >= sprite_y && ppu->draw_y < sprite_y + (ppu->controller.spr_size ? 16 : 8);
}

static inline void inc_vram_addr(ppu_t *ppu, vram_reg_t *addr) {
    if (!ppu->controller.vram_inc) {
        addr->coarse_x++;
    }
    if (ppu->controller.vram_inc || addr->coarse_x == 0) {
        addr->coarse_y++;
        if (addr->coarse_y == 0) {
            addr->nt_x++;
            if (addr->nt_x == 0) {
                addr->nt_y++;
                if (addr->nt_y == 0) {
                    addr->fine_y++;
                    if (addr->fine_y == 0) {
                        if (!ppu->controller.vram_inc) {
                            addr->coarse_x = 0;
                        }
                        addr->coarse_y = 0;
                        addr->nt_x = 0;
                        addr->nt_y = 0;
                        addr->fine_y = 0;
                    }
                }
            }
        }
    }
}

static inline void inc_vram_x(vram_reg_t *addr) {
    if (addr->coarse_x < 31) {
        addr->coarse_x++;
    }
    else {
        addr->coarse_x = 0;
        addr->nt_x = !addr->nt_x;
    }
}

static inline void inc_vram_y(vram_reg_t *addr) {
    if (addr->fine_y < 7) {
        addr->fine_y++;
    }
    else {
        addr->fine_y = 0;
        if (addr->coarse_y == 29) {
            addr->coarse_y = 0;
            addr->nt_y = !addr->nt_y;
        }
        else if (addr->coarse_y == 31) {
            addr->coarse_y = 0;
        }
        else {
            addr->coarse_y++;
        }
    }
}

ppu_t *ppu_create(void) {
    // Create the PPU.
    ppu_t *ppu = malloc(sizeof(struct ppu));
    ppu->vram = malloc(sizeof(uint8_t) * VRAM_SIZE);
    ppu->as = as_create();

    // Clear registers.
    ppu->controller.value = 0;
    ppu->mask.value = 0;
    ppu->status.value = 0;
    ppu->oam_addr = 0;
    ppu->scroll = 0;
    ppu->ppu_addr = 0;
    ppu->ppu_data = 0;
    ppu->w = 0;

    // Set the draw position to (0, 0).
    ppu->draw_x = 0;
    ppu->draw_y = 0;

    // Clear the odd frame flag.
    ppu->odd_frame = false;

    // Make the background black at the start.
    ppu->bkg_color = 0x0F;

    return ppu;
}

void ppu_destroy(ppu_t *ppu) {
    as_destroy(ppu->as);
    free(ppu->vram);
    free(ppu);
}

void ppu_reset(ppu_t *ppu) {
    // These registers are cleared on reset.
    ppu->controller.value = 0;
    ppu->mask.value = 0;
    ppu->scroll = 0;
    ppu->ppu_data = 0;
    ppu->w = 0;

    // Odd frame flag is reset.
    ppu->odd_frame = false;
}

void ppu_render(ppu_t *ppu, int cycles) {
    bool vbl_suppress = false;

    // PPUCONTROL
    if (ppu->ppucontrol_flags.write) {
        // Update nametable.
        ppu->t.nt_x = ppu->controller.nt_addr & 0x01;
        ppu->t.nt_y = (ppu->controller.nt_addr >> 1) & 0x01;

        // Update NMI status.
        if (!ppu->controller.nmi) {
            ppu->nmi_occurred = false;
        }

        ppu->ppucontrol_flags.write = 0;
    }

    // PPUSTATUS
    if (ppu->ppustatus_flags.read) {
        // Clear VBL flag and write toggle bit.
        ppu->status.vblank = 0;
        ppu->w = 0;

        // Suppress VBL flag if set in the next PPU cycle.
        vbl_suppress = true;

        ppu->ppustatus_flags.read = 0;
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
            ppu->t.coarse_x = ppu->ppu_addr & 0x1F;
            ppu->t.coarse_y = (ppu->t.coarse_y & ~0x07) | (ppu->ppu_addr >> 5);
            ppu->v = ppu->t;
        }
        else {
            // first write
            ppu->t.coarse_y = (ppu->t.coarse_y & 0x07) | ((ppu->ppu_addr & 0x03) << 3);
            ppu->t.nt_x = (ppu->ppu_addr >> 2) & 0x01;
            ppu->t.nt_y = (ppu->ppu_addr >> 3) & 0x01;
            ppu->t.fine_y = (ppu->ppu_addr >> 4) & 0x03;
        }
        ppu->ppuaddr_flags.write = 0;
        ppu->w = !ppu->w;
    }

    // PPUDATA
    if (ppu->ppudata_flags.write || ppu->ppudata_flags.read) {
        addr_t addr = (ppu->v.fine_y << 12) | (ppu->v.nt_y << 11) | (ppu->v.nt_x << 10) | (ppu->v.coarse_y << 5) | ppu->v.coarse_x;
        if (ppu->ppudata_flags.write) {
            as_write(ppu->as, addr, ppu->ppu_data);
        }
        if (ppu->ppudata_flags.read) {
            ppu->ppu_data = as_read(ppu->as, addr);
        }
        inc_vram_addr(ppu, &ppu->v);
        ppu->ppudata_flags.write = 0;
        ppu->ppudata_flags.read = 0;
    }
    
    // Rendering.
    bool rendering = ppu->mask.background || ppu->mask.sprites;
    while (cycles > 0) {
        // Render background and sprites.
        render_cycle(ppu, rendering, vbl_suppress);
        
        // Sprite evaluation.
        sprite_evaluation(ppu);

        // Increment scanline pointer.
        ppu->draw_x++;
        if (ppu->draw_x == 341) {
            ppu->draw_x = 0;
            ppu->draw_y++;
        }
        if (ppu->draw_y == 261) {
            ppu->draw_y = -1;
        }

        vbl_suppress = false;
        if (ppu->nmi_suppress > 0) {
            ppu->nmi_suppress--;
        }
        cycles--;
    }
}

static inline void render_cycle(ppu_t *ppu, bool rendering, bool vbl_suppress) {
    if (ppu->draw_y < 240) {
        if (ppu->draw_y == -1) {
            // Pre-render scanline.
            if (ppu->draw_x == 1) {
                ppu->status.vblank = 0;
                ppu->status.overflow = 0;
                ppu->status.hit = 0;
            }
            else if (ppu->draw_x >= 280 && ppu->draw_x <= 304) {
                // Reset y.
                if (rendering) {
                    ppu->v.coarse_y = ppu->t.coarse_y;
                    ppu->v.fine_y = ppu->t.fine_y;
                    ppu->v.nt_y = ppu->t.nt_y;
                }
            }
            else if (ppu->draw_x == 339) {
                // Skip a frame at x=339 on odd frames if background rendering is enabled.
                if (ppu->mask.background) {
                    if (ppu->odd_frame) {
                        ppu->draw_x++;
                    }
                }
                ppu->odd_frame = !ppu->odd_frame;
            }
        }
        else if (ppu->draw_x > 0 && ppu->draw_x <= 256) {
            // Get the x-coordinate of the pixel on the screen.
            const uint8_t screen_x = ppu->draw_x - 1;

            // Get the mask used for the shift registers.
            const uint16_t sr_mask = 0x8000 >> ppu->x;

            // Determine the value of the bit at this pixel of the tile.
            uint8_t bkg;
            if (!ppu->mask.background) {
                bkg = 0;
            }
            else if (ppu->draw_x <= 8 && !ppu->mask.bkg_left) {
                bkg = 0;
            }
            else {
                uint8_t b0 = (ppu->sr_tile[0] & sr_mask) > 0;
                uint8_t b1 = (ppu->sr_tile[1] & sr_mask) > 0;
                bkg = (b1 << 1) | b0;
            }
            
            // Determine the color of the pixel.
            uint8_t b0 = (ppu->sr_attr[0] & sr_mask) > 0;
            uint8_t b1 = (ppu->sr_attr[1] & sr_mask) > 0;
            uint8_t index = (b1 << 1) | b0;
            uint8_t col_index = bkg > 0 ? ppu->bkg_palette[index * 4 + bkg - 1] : ppu->bkg_color;

            // Check if the pixel should be overriden with one from a sprite.
            if (ppu->mask.sprites) {
                // Get active sprites.
                for (int i = 0; i < 8; i++) {
                    if (ppu->oam_x[i] == 0xFF)
                        continue;
                    if (ppu->oam_x[i] > screen_x)
                        continue;
                    if (screen_x > ppu->oam_x[i] + 8)
                        continue;
                    if (ppu->draw_x <= 8 && !ppu->mask.spr_left)
                        continue;
                    
                    // Get fine-x and flip horizontally if necessary.
                    uint8_t fine_x = screen_x - ppu->oam_x[i];
                    if (ppu->oam_attr[i].flip_h) {
                        fine_x = 7 - fine_x;
                    }

                    // Get value of sprite at current pixel and decide whether it should override the background.
                    uint8_t mask = 0x80 >> fine_x;
                    uint8_t low = (ppu->oam_p[i][0] & mask) > 0;
                    uint8_t high = (ppu->oam_p[i][1] & mask) > 0;
                    uint8_t spr = (high << 1) | low;
                    if (spr == 0)
                        continue;
                    
                    // Check if the sprite 0 hit flag should be updated.
                    if (i == 0 && bkg > 0 && ppu->szc && ppu->draw_x != 256) {
                        ppu->status.hit = 1;
                    }

                    // At this stage, if there is an opaque background pixel and the sprite's priority bit is set,
                    // then the background pixel should be drawn regardless of the priority of any remaining sprites.
                    if (bkg > 0 && ppu->oam_attr[i].priority)
                        break;
                    
                    // Update the color and break as any subsequent sprites would be displayed behind this sprite.
                    col_index = ppu->spr_palette[ppu->oam_attr[i].palette * 3 + spr - 1];
                    break;
                }
            }

            // Output the pixel.
            color_t col = color_resolve(col_index);
            put_pixel(ppu, screen_x, ppu->draw_y, col);
        }

        // Tile fetches.
        if ((ppu->draw_x > 0 && ppu->draw_x <= 256) || (ppu->draw_x > 320 && ppu->draw_x <= 336)) {
            // Clock shift registers.
            ppu->sr_attr[0] <<= 1;
            ppu->sr_attr[1] <<= 1;
            ppu->sr_tile[0] <<= 1;
            ppu->sr_tile[1] <<= 1;
        
            // Do tile fetches if necessary.
            if (ppu->draw_x % 8 == 0) {
                // Fetch high BG tile byte.
                ppu->nt_latch.plane = 1;
                addr_t pt_addr = get_pt_addr(ppu->nt_latch);
                ppu->tile_latch[1] = as_read(ppu->as, pt_addr);
                
                if (rendering) {
                    if (ppu->draw_x == 256) {
                        // Increment y.
                        inc_vram_y(&ppu->v);
                    }
                    else {
                        // Increment x.
                        inc_vram_x(&ppu->v);
                    }
                }

                // Reload shift registers (for next cycle).
                bool attr_low = (ppu->attr_latch & 0x01) > 0;
                bool attr_high = (ppu->attr_latch & 0x02) > 0;
                
                ppu->sr_attr[0] = (ppu->sr_attr[0] & 0xFF00) | (attr_low ? 0xFF : 0x00);
                ppu->sr_attr[1] = (ppu->sr_attr[1] & 0xFF00) | (attr_high ? 0xFF : 0x00);

                ppu->sr_tile[0] = (ppu->sr_tile[0] & 0xFF00) | ppu->tile_latch[0];
                ppu->sr_tile[1] = (ppu->sr_tile[1] & 0xFF00) | ppu->tile_latch[1];
            }
            else if (ppu->draw_x % 8 == 2) {
                // Fetch NT byte.
                addr_t nt_addr = get_nt_addr(ppu->v);
                uint8_t tile = as_read(ppu->as, nt_addr);
                ppu->nt_latch.tile_x = tile & 0x0F;
                ppu->nt_latch.tile_y = (tile & 0xF0) >> 4;
                ppu->nt_latch.table = ppu->controller.bpt_addr;
                ppu->nt_latch.fine_y = ppu->v.fine_y;
            }
            else if (ppu->draw_x % 8 == 4) {
                // Fetch AT byte.
                addr_t attr_addr = get_at_addr(ppu->v);
                uint8_t attr = as_read(ppu->as, attr_addr);
                if ((ppu->v.coarse_x & 0x02) > 0)
                    attr >>= 2;
                if ((ppu->v.coarse_y & 0x02) > 0)
                    attr >>= 4;
                ppu->attr_latch = attr & 0x03;
            }
            else if (ppu->draw_x % 8 == 6) {
                // Fetch low BG tile byte.
                ppu->nt_latch.plane = 0;
                addr_t pt_addr = get_pt_addr(ppu->nt_latch);
                ppu->tile_latch[0] = as_read(ppu->as, pt_addr);
            }
        }
        else if (ppu->draw_x == 257 && rendering) {
            // Reset x.
            ppu->v.coarse_x = ppu->t.coarse_x;
            ppu->v.nt_x = ppu->t.nt_x;
        }
        else if (ppu->draw_x == 338 || ppu->draw_x == 340) {
            // Unused NT fetches.
            addr_t attr_addr = get_at_addr(ppu->v);
            as_read(ppu->as, attr_addr);
        }
    }
    else if (ppu->draw_y == 241) {
        // Vblank.
        if (ppu->draw_x == 1) {
            // Set VBL flag if not suppressed.
            ppu->status.vblank = !vbl_suppress;

            // Suppress NMI for 3 PPU cycles (1 CPU cycle) after reading.
            ppu->nmi_suppress = 3;

            ppu->vbl_occurred = true;
            ppu->nmi_occurred = false;
        }
    }
}

static inline void sprite_evaluation(ppu_t *ppu) {
    if (ppu->draw_y < 240) {
        if (ppu->draw_x == 0) {
            // Idle (reset iterators).
            ppu->n = 0;
            ppu->m = 0;
            ppu->szn = false;
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
                    if (ppu->mask.background || ppu->mask.sprites) {
                        ppu->status.overflow = true; // Shouldn't be set if all rendering is off.
                    }
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
                    if (ppu->n == 0) {
                        ppu->szn = true;
                    }
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

            // Check if sprite 0 is included at indices 0-3 of the secondary OAM.
            ppu->szc = ppu->szn;
        }
    }
}
