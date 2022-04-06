#ifndef PPU_H
#define PPU_H

#include <stdint.h>
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

typedef struct palette {

    uint8_t         col1;
    uint8_t         col2;
    uint8_t         col3;

} palette_t;

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

    uint8_t         bkg_color;              // The universal background color;   
    palette_t       palettes[N_PALETTES];   

} ppu_t;

#endif

ppu_t *ppu_create(void);

void ppu_destroy(ppu_t *ppu);

