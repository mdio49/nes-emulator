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

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);

const mapper_t uxrom = {
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
    mapper->data = NULL;
    
    return mapper;
}

static void insert(mapper_t *mapper, prog_t *prog) {
    // First PRG bank is switchable (map to the start of PRG-ROM).
    as_add_segment(mapper->cpuas, PRG_BANK0, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ);

    // Second PRG bank is fixed to the last bank.
    const int nbanks = prog->header.prg_rom_size * INES_PRG_ROM_UNIT / PRG_BANK_SIZE;
    as_add_segment(mapper->cpuas, PRG_BANK1, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom + (nbanks - 1) * PRG_BANK_SIZE, AS_READ);

    // CHR-ROM/RAM is fixed.
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

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    // If the address targets the first bank, then offset target depending on the value in the bank register.
    return vaddr < PRG_BANK1 ? target + mapper->banks[0] * PRG_BANK_SIZE : target;
}

static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    return target; // CHR memory is fixed.
}

static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    return target; // Nametables are fixed.
}

static void write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value) {
    if (vaddr < PRG_ROM_START)
        return;
    
    // Bank 0 simply gets set to the value given.
    mapper->banks[0] = value;
}