#ifndef INES_H
#define INES_H

#include <stdint.h>

#define INES_HEADER_SIZE    16
#define INES_TRAINER_SIZE   512
#define INES_PRG_ROM_UNIT   16384
#define INES_PGR_RAM_UNIT   8192
#define INES_CHR_ROM_UNIT   8192
#define INES_INST_ROM_SIZE  8192
#define INES_PROM_SIZE      16

typedef union ines_flag6 {
    struct flags6 {
        unsigned    mirroring   : 1;
        unsigned    prg_ram     : 1;
        unsigned    trainer     : 1;
        unsigned    four_screen : 1;
        unsigned    mapper_low  : 4;
    } flags;
    uint8_t bits;
} ines_flag6_t;

typedef union ines_flag7 {
    struct flags7 {
        unsigned    vs_unisys       : 1;
        unsigned    playchoice_10   : 1;
        unsigned    format          : 2;
        unsigned    mapper_high     : 4;
    } flags;
    uint8_t bits;
} ines_flag7_t;

typedef union ines_flag8 {
    struct flags8 {
        unsigned    prg_ram_size    : 8;
    } flags;
    uint8_t bits;
} ines_flag8_t;

typedef union ines_flag9 {
    struct flags9 {
        unsigned    tv_sys  : 1;
        unsigned            : 7;
    } flags;
    uint8_t bits;
} ines_flag9_t;

typedef struct ines_header {
    
    uint8_t     prg_rom_size;           // size of PRG-ROM, in 16KB units
    uint8_t     chr_rom_size;           // size of CHR-ROM, in 8KB units (if 0, then CHR-RAM is used)
    uint8_t     prg_ram_size;           // size of PRG-RAM, in 8KB units (if used)
    uint16_t    mapper_no;              // mapper number

    unsigned    mirroring       : 1;    // 0: horizontal, 1: vertical
    unsigned    prg_ram         : 1;    // whether cartridge contains battery-backed PGR-RAM
    unsigned    trainer         : 1;    // whether cartridge contains 512-byte "trainer"
    unsigned    four_screen     : 1;    // whether to ignore mirroring control and provide 4-screen VRAM

    unsigned    vs_unisys       : 1;    // VS unisystem
    unsigned    playchoice_10   : 1;    // 8KB of "hint screen" data stored after CHR data
    unsigned    format          : 2;    // format of ROM; if 2, then NES 2.0 is used

    unsigned    tv_sys          : 1;    // 0: NTSC, 1: PAL

} ines_header_t;

#endif
