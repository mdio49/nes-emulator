#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <vm.h>

#define VRAM_SIZE   0x0800

#define PPU_CTRL    0x2000
#define PPU_MASK    0x2001
#define PPU_STATUS  0x2002
#define OAM_ADDR    0x2003
#define OAM_DATA    0x2004
#define PPU_SCROLL  0x2005
#define PPU_ADDR    0x2006
#define PPU_DATA    0x2007

#define SCREEN_WIDTH    256
#define SCREEN_HEIGHT   240

#define NT_ROWS     30
#define NT_COLS     32

#define N_SPRITES   64

#define OUT_R       0x00
#define OUT_G       0x01
#define OUT_B       0x02

#define SCANLINE_END    340
#define N_SCANLINES     260

typedef struct {
    unsigned    read    : 1;
    unsigned    write   : 1;
    unsigned            : 6;
} io_flags_t;

/**
 * @brief A pattern table entry.
 */
typedef union pt_entry {
    struct {
        unsigned    fine_x  : 3;    // Fine X offset (column number within tile).
        unsigned    fine_y  : 3;    // Fine Y offset (row number within tile).
        unsigned    plane   : 1;    // Bit plane (0 = lower; 1 = upper).
        unsigned    tile_x  : 4;    // Tile column.
        unsigned    tile_y  : 4;    // Tile row.
        unsigned    table   : 1;    // Table to use (0 = left; 1 = right).
    };
    struct {
        unsigned            : 3;
        unsigned    addr    : 13;   // The resolved address.
    };
} pt_entry_t;

typedef struct nt_entry {
    unsigned    cell_x  : 5;    // The x-coordinate of the cell along the nametable.
    unsigned    cell_y  : 5;    // The y-coordinate of the cell along the nametable.
    unsigned    nt_x    : 1;    // Nametable along x-direction (0: left; 1: right).
    unsigned    nt_y    : 1;    // Nametable along y-direction (0: top; 1: bottom).
    unsigned            : 4;
} nt_entry_t;

typedef struct vram_reg {
    unsigned    coarse_x    : 5;
    unsigned    coarse_y    : 5;
    unsigned    nt_x        : 1;
    unsigned    nt_y        : 1;
    unsigned    fine_y      : 3;
    unsigned                : 1;
} vram_reg_t;

typedef struct spr_attr {

    unsigned    palette     : 2;    // Palette number (4 to 7).
    unsigned                : 3;
    unsigned    priority    : 1;    // Priority (0: in front of background; 1: behind background).
    unsigned    flip_h      : 1;    // Flip sprite horizontally.
    unsigned    flip_v      : 1;    // Flip sprite vertically.

} spr_attr_t;

/**
 * @brief A PPU struct that contains all data needed to emulate the PPU.
 */
typedef struct ppu {

    /* main registers/memory */
    
    vram_reg_t      v;                  // Current VRAM address.
    vram_reg_t      t;                  // Temporary VRAM address.

    uint8_t         x : 3;              // Fine X scroll.
    uint8_t         w : 1;              // First or second write toggle bit.
    unsigned          : 4;
    
    uint16_t        sr16[2];            // 16-bit shift registers.
    uint8_t         sr8[2];             // 8-bit shift registers.

    addrspace_t     *as;                // The PPU's address space.
    uint8_t         *vram;              // 2KB of memory.

    uint8_t         bkg_color;          // The universal background color.
    uint8_t         bkg_palette[15];    // Background palette memory.
    uint8_t         spr_palette[12];    // Sprite palette memory.

    uint8_t         oam[256];           // Object attribute memory.
    uint8_t         oam2[32];           // Secondary OAM.

    /* oam shift registers */

    uint8_t         oam_p[8][2];        // Sprite tile planes.
    uint8_t         oam_x[8];           // Sprite x position.
    spr_attr_t      oam_attr[8];        // Sprite attribute memory.

    /* variables used during sprite evaluation */

    unsigned        n   : 8;            // n-iterator for sprite evaluation.
    unsigned        m   : 2;            // m-iterator for sprite evaluation.
    unsigned        s0  : 1;            // Set if sprite 0 is included in the current scanline.
    unsigned            : 5;

    uint8_t         oam2_ptr;           // Pointer to secondary OAM memory during sprite evaluation.
    uint8_t         oam_buffer;         // OAM read buffer during sprite evaluation.

    /* memory mapped registers (stored contiguously) */
    
    // PPUCTRL
    union ppu_ctrl {
        struct {
            unsigned    nt_addr     : 2;    // Base nametable address (0: $2000; 1: $2400; 2: $2800; 3: $2C00).
            unsigned    vram_inc    : 1;    // VRAM address increment per CPU read/write of PPU data (0: add 1; 1: add 32).
            unsigned    spt_addr    : 1;    // Sprite pattern table address (0: $0000; 1: $1000; ignored in 8x16 mode).
            unsigned    bpt_addr    : 1;    // Background pattern table address (0: $0000; 1: $1000).
            unsigned    spr_size    : 1;    // Sprite size (0: 8x8; 1: 8x16).
            unsigned    m_slave     : 1;    // PPU master/slave select.
            unsigned    nmi         : 1;    // Generate NMI at start of vblank.
        };
        uint8_t value;
    } controller;

    // PPUMASK
    union ppu_mask {
        struct {
            unsigned    grayscale   : 1;    // Enable greyscale.
            unsigned    bkg_left    : 1;    // Show background in leftmost 8 pixels of screen.
            unsigned    spr_left    : 1;    // Show sprites in leftmost 8 pixels of screen.
            unsigned    background  : 1;    // Show background.
            unsigned    sprites     : 1;    // Show sprites.
            unsigned    em_red      : 1;    // Emphasize red (green on PAL).
            unsigned    em_green    : 1;    // Emphasize green (red on PAL).
            unsigned    em_blue     : 1;    // Emphasize blue.
        };
        uint8_t value;
    } mask;

    // PPUSTATUS
    union ppu_status {
        struct {
            unsigned                : 5;
            unsigned    overflow    : 1;    // Sprite overflow.
            unsigned    hit         : 1;    // Sprite 0 hit.
            unsigned    vblank      : 1;    // Vertical blank.
        };
        uint8_t value;
    } status;

    // OAMADDR
    uint8_t     oam_addr;

    // OAMDATA
    uint8_t     oam_data;

    // PPUSCROLL
    uint8_t     scroll; 

    // PPUADDR
    uint8_t     ppu_addr;

    // PPUDATA
    uint8_t     ppu_data;

    /* io flags */

    io_flags_t  ppucontrol_flags;
    io_flags_t  ppustatus_flags;
    io_flags_t  oamaddr_flags;
    io_flags_t  oamdata_flags;
    io_flags_t  ppuscroll_flags;
    io_flags_t  ppuaddr_flags;
    io_flags_t  ppudata_flags;

    /* variables used for background rendering */

    int16_t     draw_x, draw_y;                     // Current screen position of render.
    char out[SCREEN_WIDTH * SCREEN_HEIGHT * 3];     // Pixel output (3 bytes per pixel; RGB order).

    unsigned    nmi_occurred    : 1;                // A flag that is true if an NMI should occur on the next CPU instruction fetch.
    unsigned    vbl_occurred    : 1;                // A flag that is true if a vblank just occured and the screen should be redrawn.
    unsigned                    : 6;

    /* temporary until interrupts are done correctly */

    addr_t      vram_write_addr;

} ppu_t;

#endif

ppu_t *ppu_create(void);

void ppu_destroy(ppu_t *ppu);

void ppu_render(ppu_t *ppu, int cycles);
