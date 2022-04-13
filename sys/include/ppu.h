#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <vm.h>

#define N_PALETTES  8
#define VRAM_SIZE   0x0800

#define PPU_CTRL    0x2000
#define PPU_MASK    0x2001
#define PPU_STATUS  0x2002
#define OAM_ADDR    0x2003
#define OAM_DATA    0x2004
#define PPU_SCROLL  0x2005
#define PPU_ADDR    0x2006
#define PPU_DATA    0x2007
#define OAM_DMA     0x4014

#define TIME_STEP   0.05 * CLOCKS_PER_SEC

typedef struct palette {

    uint8_t         col1;
    uint8_t         col2;
    uint8_t         col3;

} palette_t;

typedef union io_flags {
    struct {
        unsigned    read    : 1;
        unsigned    write   : 1;
        unsigned    low     : 1;
        unsigned            : 5;
    };
    uint8_t value;
} io_flags_t;

/**
 * @brief A PPU struct that contains all data needed to emulate the PPU.
 */
typedef struct ppu {
    
    uint16_t        v : 15;         // Current VRAM address.
    uint16_t        t : 15;         // Temporary VRAM address.
    unsigned          : 2;

    uint8_t         x : 3;          // Fine X scroll.
    uint8_t         w : 1;          // First or second write toggle bit.
    unsigned          : 4;
    
    uint16_t        sr16[2];        // 16-bit shift registers.
    uint8_t         sr8[2];         // 8-bit shift registers.

    addrspace_t     *as;            // The PPU's address space.
    uint8_t         *vram;          // 2KB of memory.

    uint8_t         bkg_color;              // The universal background color.
    palette_t       palettes[N_PALETTES];

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

    io_flags_t  ppustatus_flags;
    io_flags_t  oamaddr_flags;
    io_flags_t  oamdata_flags;
    io_flags_t  ppuaddr_flags;
    io_flags_t  ppudata_flags;

    const char  *out;

    /* temporary until cycling is done correctly */

    clock_t last_time;
    clock_t frame_counter;
    bool    flush_flag;

} ppu_t;

#endif

ppu_t *ppu_create(void);

void ppu_destroy(ppu_t *ppu);

void ppu_render(ppu_t *ppu, int cycles);
