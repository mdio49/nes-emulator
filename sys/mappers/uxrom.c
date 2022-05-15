#include <mappers.h>
#include <stdio.h>
#include <stdlib.h>
#include <ppu.h>

#define N_REGISTERS     1

#define CHR_BANK0       0x0000
#define CHR_BANK_SIZE   0x2000

#define PRG_BANK0       0x8000
#define PRG_BANK1       0xC000
#define PRG_BANK_SIZE   0x4000

static mapper_t *init(void);

static void insert(mapper_t *mapper, prog_t *prog);
static void monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write);

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);

const mapper_t uxrom = {
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
    
    /* setup registers */
    mapper->banks = calloc(N_REGISTERS, sizeof(uint8_t));
    
    return mapper;
}

static void insert(mapper_t *mapper, prog_t *prog) {
    // First PRG bank is switchable (map to the start of PRG-ROM).
    as_add_segment(mapper->cpuas, PRG_BANK0, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ);

    // Second PRG bank is fixed to the last bank.
    as_add_segment(mapper->cpuas, PRG_BANK1, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom + (N_PRG_BANKS(prog, PRG_BANK_SIZE) - 1) * PRG_BANK_SIZE, AS_READ);

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

static void monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write) {
    if (!write)
        return;
    if (as != mapper->cpuas)
        return;
    if (vaddr < PRG_ROM_START)
        return;
    
    // Bank 0 simply gets set to the value given.
    mapper->banks[0] = value;
}

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    // If the address targets the first bank, then offset target depending on the value in the bank register.
    return vaddr < PRG_BANK1 ? target + mapper->banks[0] * PRG_BANK_SIZE : target;
}

