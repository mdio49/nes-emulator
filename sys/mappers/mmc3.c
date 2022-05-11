#include <mappers.h>
#include <stdio.h>
#include <stdlib.h>
#include <ppu.h>

/**
 * bank registers:
 * 0    - bank select
 * 1..6 - chr baks
 * 7..8 - prg banks
 * 9    - mirroring
 * 10   - prg-ram protect
 * 11   - irq latch
 */

#define N_BANKS         16
#define SELECT_INDEX    0    
#define BANKS_INDEX     1   
#define MIRROR_INDEX    9
#define PROTECT_INDEX   10

#define R0  (BANKS_INDEX + 0)
#define R1  (BANKS_INDEX + 1)
#define R2  (BANKS_INDEX + 2)
#define R3  (BANKS_INDEX + 3)
#define R4  (BANKS_INDEX + 4)
#define R5  (BANKS_INDEX + 5)
#define R6  (BANKS_INDEX + 6)
#define R7  (BANKS_INDEX + 7)

#define CHR_BANK0       0x0000
#define CHR_BANK_SIZE   0x0400

#define PRG_RAM         0x6000
#define PRG_RAM_SIZE    0x2000

#define PRG_BANK0       0x8000
#define PRG_BANK1       0xA000
#define PRG_BANK2       0xC000
#define PRG_BANK3       0xE000
#define PRG_BANK_SIZE   0x2000

struct mmc3_data {

    uint8_t     irq_counter;        // irq counter
    uint8_t     irq_latch;          // irq reload value
    unsigned    irq_enable  : 1;    // irq enable flag
    unsigned    irq_reload  : 1;    // irq reload flag
    unsigned                : 6;

};

static mapper_t *init(void);

static void insert(mapper_t *mapper, prog_t *prog);
static void write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value);

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);

const mapper_t mmc3 = {
    .init = init
};

static mapper_t *init(void) {
    /* create mapper */
    mapper_t *mapper = malloc(sizeof(struct mapper));

    /* set functions */
    mapper->insert = insert;
    mapper->write = write;
    mapper->map_prg = map_prg;
    mapper->map_chr = map_chr;
    mapper->map_nts = map_nts;
    
    /* setup registers */
    mapper->banks = calloc(N_BANKS, sizeof(uint8_t));

    /* additional data */
    mapper->data = malloc(sizeof(struct mmc3_data));
    
    return mapper;
}

static void insert(mapper_t *mapper, prog_t *prog) {
    // Fixed 8KB of PRG-RAM (may not be used).
    as_add_segment(mapper->cpuas, PRG_RAM, PRG_RAM_SIZE, prog->prg_ram, AS_READ | AS_WRITE);
    
    // Three switchable PRG-ROM banks (one is fixed to the second last bank).
    as_add_segment(mapper->cpuas, PRG_BANK0, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ);
    as_add_segment(mapper->cpuas, PRG_BANK1, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ);
    as_add_segment(mapper->cpuas, PRG_BANK2, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ);

    // Fourth bank is fixed to the last bank.
    const int nbanks = prog->header.prg_rom_size * INES_PRG_ROM_UNIT / PRG_BANK_SIZE;
    as_add_segment(mapper->cpuas, PRG_BANK3, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom + (nbanks - 1) * PRG_BANK_SIZE, AS_READ);

    // Have 8 separate 1KB CHR segments for both possible arrangements of CHR banks.
    for (int i = 0; i < 8; i++) {
        if (prog->chr_rom != NULL) {
            as_add_segment(mapper->ppuas, CHR_BANK0 + i * CHR_BANK_SIZE, CHR_BANK_SIZE, (uint8_t*)prog->chr_rom, AS_READ);
        }
        else {
            as_add_segment(mapper->ppuas, CHR_BANK0 + i * CHR_BANK_SIZE, CHR_BANK_SIZE, prog->chr_ram, AS_READ | AS_WRITE);
        }
    }

    // Map each nametable to the start of VRAM.
    as_add_segment(mapper->ppuas, NAMETABLE0, NT_SIZE, mapper->vram, AS_READ | AS_WRITE);
    as_add_segment(mapper->ppuas, NAMETABLE1, NT_SIZE, mapper->vram, AS_READ | AS_WRITE);
    as_add_segment(mapper->ppuas, NAMETABLE2, NT_SIZE, mapper->vram, AS_READ | AS_WRITE); 
    as_add_segment(mapper->ppuas, NAMETABLE3, NT_SIZE, mapper->vram, AS_READ | AS_WRITE);
}

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    // Last bank is always fixed.
    if (vaddr >= PRG_BANK3)
        return target;
    
    // Map depending on the PRG banking mode.
    if ((mapper->banks[SELECT_INDEX] & 0x40) > 0) {
        // Bank 0 fixed to second last bank.
        if (vaddr < PRG_BANK1) {        // Bank 0
            const int nbanks = prog->header.prg_rom_size * INES_PRG_ROM_UNIT / PRG_BANK_SIZE;
            target += (nbanks - 2) * PRG_BANK_SIZE;
        }
        else if (vaddr < PRG_BANK2) {   // Bank 1
            target += mapper->banks[R7] * PRG_BANK_SIZE;
        }
        else {                          // Bank 2
            target += mapper->banks[R6] * PRG_BANK_SIZE;
        }
    }
    else {
        // Bank 2 fixed to second last bank.
        if (vaddr < PRG_BANK1) {        // Bank 0
            target += mapper->banks[R6] * PRG_BANK_SIZE;
        }
        else if (vaddr < PRG_BANK2) {   // Bank 1
            target += mapper->banks[R7] * PRG_BANK_SIZE;
        }
        else {                          // Bank 2
            const int nbanks = prog->header.prg_rom_size * INES_PRG_ROM_UNIT / PRG_BANK_SIZE;
            target += (nbanks - 2) * PRG_BANK_SIZE;
        }
    }

    return target;
}

static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    uint8_t bank = vaddr / CHR_BANK_SIZE;
    if ((mapper->banks[0] & 0x80) > 0) {
        // R2 - R3 - R4 - R5 - R0/R0 - R1/R1
        if (bank < 4) {
            target += mapper->banks[R2 + bank] * CHR_BANK_SIZE;
        }
        else if (bank < 6) {
            target += (mapper->banks[R0] + (bank % 2)) * CHR_BANK_SIZE;
        }
        else {
            target += (mapper->banks[R1] + (bank % 2)) * CHR_BANK_SIZE;
        }
    }
    else {
        // R0/R0 - R1/R1 - R2 - R3 - R4 - R5
        if (bank < 2) {
            target += (mapper->banks[R0] + (bank % 2)) * CHR_BANK_SIZE;
        }
        else if (bank < 4) {
            target += (mapper->banks[R1] + (bank % 2)) * CHR_BANK_SIZE;
        }
        else {
            target += mapper->banks[R2 + (bank - 4)] * CHR_BANK_SIZE;
        }
    }
    return target;
}

static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    uint8_t nt = NT(vaddr);
    if ((mapper->banks[MIRROR_INDEX] & 0x01) > 0) {
        // Horizontal mirroring.
        if (nt == 2 || nt == 3) {
            target += NT_SIZE;
        }
    }
    else {
        // Vertical mirroring.
        if (nt == 1 || nt == 3) {
            target += NT_SIZE;
        }
    }
    return target;
}

static void write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value) {
    if (vaddr < PRG_ROM_START)
        return;

    if (vaddr % 2 == 0) {
        // even addresses
        if (vaddr < 0xA000) {
            // bank select
            mapper->banks[0] = value;
        }
        else if (vaddr < 0xC000) {
            // mirroring
            mapper->banks[MIRROR_INDEX] = value;
        }
        else if (vaddr < 0xE000) {
            // irq latch
            ((struct mmc3_data*)mapper->data)->irq_latch = value;
        }
        else {
            // irq disable
            ((struct mmc3_data*)mapper->data)->irq_enable = false;
        }
    }
    else {
        // odd addresses
        if (vaddr < 0xA000) {
            // bank data
            uint8_t r = mapper->banks[0] & 0x07;
            mapper->banks[R0 + r] = value;
        }
        else if (vaddr < 0xC000) {
            // prg-ram protect (not used)
            mapper->banks[PROTECT_INDEX] = value;
        }
        else if (vaddr < 0xE000) {
            // irq reload
            ((struct mmc3_data*)mapper->data)->irq_reload = true;
        }
        else {
            // irq enable
            ((struct mmc3_data*)mapper->data)->irq_enable = true;
        }
    }
}
