#include <mappers.h>
#include <stdio.h>
#include <stdlib.h>
#include <ppu.h>

#define N_BANKS 1

#define CHR_BANK0       0x0000
#define CHR_BANK_SIZE   0x2000

#define PRG_BANK0       0x8000
#define PRG_BANK1       0xC000
#define PRG_BANK_SIZE   0x4000

static mapper_t *init(void);

static void insert(mapper_t *mapper, prog_t *prog);
static void write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value);

static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);

const mapper_t ines003 = {
    .init = init
};

static mapper_t *init(void) {
    /* create mapper */
    mapper_t *mapper = mapper_create();

    /* set functions */
    mapper->insert = insert;
    mapper->write = write;

    /* set mapper rules */
    mapper->map_chr = map_chr;
    
    /* setup registers */
    mapper->banks = calloc(N_BANKS, sizeof(uint8_t));
    
    return mapper;
}

static void insert(mapper_t *mapper, prog_t *prog) {
    // PRG-ROM is fixed (mirrored if there is only one 16KB bank).
    as_add_segment(mapper->cpuas, PRG_BANK0, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ);
    if (prog->header.prg_rom_size > 1) {
        as_add_segment(mapper->cpuas, PRG_BANK1, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom + PRG_BANK_SIZE, AS_READ);
    }
    else {
        as_add_segment(mapper->cpuas, PRG_BANK1, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ);
    }

    // CHR-ROM/RAM (single switchable 8KB bank).
    if (prog->chr_rom != NULL) {
        as_add_segment(mapper->ppuas, CHR_BANK0, CHR_BANK_SIZE, (uint8_t*)prog->chr_rom, AS_READ);
    }
    else {
        as_add_segment(mapper->ppuas, CHR_BANK0, CHR_BANK_SIZE, prog->chr_ram, AS_READ | AS_WRITE);
    }

    // Name tables are fixed.
    as_add_segment(mapper->ppuas, NAMETABLE0, NT_SIZE, mapper->vram, AS_READ | AS_WRITE);
    if (prog->header.mirroring == 1) {
        // Vertical mirroring.
        as_add_segment(mapper->ppuas, NAMETABLE1, NT_SIZE, mapper->vram + NT_SIZE, AS_READ | AS_WRITE);
        as_add_segment(mapper->ppuas, NAMETABLE2, NT_SIZE, mapper->vram, AS_READ | AS_WRITE);
        as_add_segment(mapper->ppuas, NAMETABLE3, NT_SIZE, mapper->vram + NT_SIZE, AS_READ | AS_WRITE);
    }
    else {
        // Horizontal mirroring.
        as_add_segment(mapper->ppuas, NAMETABLE1, NT_SIZE, mapper->vram, AS_READ | AS_WRITE);
        as_add_segment(mapper->ppuas, NAMETABLE2, NT_SIZE, mapper->vram + NT_SIZE, AS_READ | AS_WRITE);
        as_add_segment(mapper->ppuas, NAMETABLE3, NT_SIZE, mapper->vram + NT_SIZE, AS_READ | AS_WRITE);
    }
}

static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    return target + mapper->banks[0] * CHR_BANK_SIZE; // Offset based on value in bank register.
}

static void write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value) {
    if (vaddr < PRG_ROM_START)
        return;
    
    // Bank 0 gets set to the lower 2 bits of the value given.
    mapper->banks[0] = value & 0x03;
}
