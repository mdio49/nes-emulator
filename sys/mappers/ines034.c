


#include <mappers.h>
#include <stdio.h>
#include <stdlib.h>
#include <ppu.h>

/**
 * bank registers:
 * 0 - control
 * 1 - CHR bank 0
 * 2 - CHR bank 1
 * 3 - PRG bank
 */

#define N_REGISTERS     3
#define PRG_SELECT      0
#define CHR_SELECT0     1
#define CHR_SELECT1     2

#define CHR_BANK0       0x0000
#define CHR_BANK1       0x1000
#define CHR_BANK_SIZE   0x1000

#define PRG_RAM         0x6000
#define PRG_RAM_SIZE    0x2000

#define PRG_BANK0       0x8000
#define PRG_BANK_SIZE   0x8000

static mapper_t *init(void);

static void insert(mapper_t *mapper, prog_t *prog);
static void monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write);

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);

const mapper_t ines034 = {
    .init = init
};

static mapper_t *init(void) {
    /* create mapper */
    mapper_t *mapper = mapper_create();

    /* set functions */
    mapper->insert = insert;
    mapper->monitor = monitor;

    /* set mapper rules */
    mapper->map_prg = map_prg;
    mapper->map_chr = map_chr;
    
    /* setup registers */
    mapper->banks = calloc(N_REGISTERS, sizeof(uint8_t));
    mapper->banks[PRG_SELECT] = 0;
    mapper->banks[CHR_SELECT0] = 0;
    mapper->banks[CHR_SELECT1] = 0;
    
    return mapper;
}

static void insert(mapper_t *mapper, prog_t *prog) {
    // Determine whether BNROM or NINA-001 is being used.
    const bool bnrom = prog->header.chr_rom_size < 2;

    // Fixed 8KB of PRG-RAM (only used in NINA-001).
    if (!bnrom) {
        prog->prg_ram = malloc(PRG_RAM_SIZE * sizeof(uint8_t));
        as_add_segment(mapper->cpuas, PRG_RAM, PRG_RAM_SIZE, prog->prg_ram, AS_READ | AS_WRITE);
    }

    // Single fixed 32KB PRG-ROM bank.
    as_add_segment(mapper->cpuas, PRG_BANK0, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ);

    // Map each CHR bank to the start of the CHR-ROM (or CHR-RAM if it is used).
    if (prog->chr_rom != NULL) {
        // NINA-001 uses two switchable 4KB CHR-ROM banks.
        as_add_segment(mapper->ppuas, CHR_BANK0, CHR_BANK_SIZE, (uint8_t*)prog->chr_rom, AS_READ);
        as_add_segment(mapper->ppuas, CHR_BANK1, CHR_BANK_SIZE, (uint8_t*)prog->chr_rom, AS_READ);
    }
    else {
        // BNROM uses a single fixed 8KB CHR-RAM bank.
        as_add_segment(mapper->ppuas, CHR_BANK0, CHR_BANK_SIZE * 2, prog->chr_ram, AS_READ | AS_WRITE);
    }

    // Name tables.
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

    // Record which mapper is being used in one of the 8-bit registers.
    mapper->r8[0] = bnrom;
}

static void monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write) {
    if (!write)
        return;
    if (as != mapper->cpuas)
        return;

    if (vaddr == 0x7FFD) {
        mapper->banks[PRG_SELECT] = value & 0x01;
    }
    else if (vaddr == 0x7FFE) {
        mapper->banks[CHR_SELECT0] = value & 0x0F;
    }
    else if (vaddr == 0x7FFF) {
        mapper->banks[CHR_SELECT1] = value & 0x0F;
    }
    else if (vaddr >= PRG_ROM_START) {
        mapper->banks[PRG_SELECT] = value & 0x03;
    }
}

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    return target + mapper->banks[PRG_SELECT] * PRG_BANK_SIZE;
}

static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    uint8_t bank = vaddr < CHR_BANK1 ? CHR_SELECT0 : CHR_SELECT1;
    return target + mapper->banks[bank] * CHR_BANK_SIZE;
}
