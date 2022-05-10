#include <mappers.h>
#include <stdlib.h>
#include <ppu.h>

static mapper_t *init(void);
static void insert(mapper_t *mapper, prog_t *prog);
static void write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value);
static uint8_t *map(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *start, size_t offset);

const mapper_t nrom = {
    .init = init
};

static mapper_t *init(void) {
    /* create mapper */
    mapper_t *mapper = malloc(sizeof(struct mapper));

    /* set functions */
    mapper->insert = insert;
    mapper->write = write;
    mapper->map_prg = map;
    mapper->map_chr = map;
    mapper->map_nts = map;
    
    /* setup registers */
    mapper->banks = NULL;
    
    return mapper;
}

static void insert(mapper_t *mapper, prog_t *prog) {
    /* Setup CPU address space. */

    // PRG-RAM.
    as_add_segment(mapper->cpuas, 0x6000, 0x2000, prog->prg_ram);

    // PRG-ROM (mirrored if there is only one 16KB bank).
    as_add_segment(mapper->cpuas, 0x8000, 0x4000, (uint8_t*)prog->prg_rom);
    if (prog->header.prg_rom_size > 1) {
        as_add_segment(mapper->cpuas, 0xC000, 0x4000, (uint8_t*)prog->prg_rom + 0x4000);
    }
    else {
        as_add_segment(mapper->cpuas, 0xC000, 0x4000, (uint8_t*)prog->prg_rom);
    }

    /* Setup PPU address space. */
    
    // Pattern tables.
    if (prog->chr_rom != NULL) {
        as_add_segment(mapper->ppuas, 0x0000, 0x2000, (uint8_t*)prog->chr_rom);
    }
    else {
        as_add_segment(mapper->ppuas, 0x0000, 0x2000, prog->chr_ram);
    }

    // Name tables.
    as_add_segment(mapper->ppuas, NAMETABLE0, NT_SIZE, mapper->vram);
    if (prog->header.mirroring == 1) {
        // Vertical mirroring.
        as_add_segment(mapper->ppuas, NAMETABLE1, NT_SIZE, mapper->vram + NT_SIZE);
        as_add_segment(mapper->ppuas, NAMETABLE2, NT_SIZE, mapper->vram);
        as_add_segment(mapper->ppuas, NAMETABLE3, NT_SIZE, mapper->vram + NT_SIZE);
    }
    else {
        // Horizontal mirroring.
        as_add_segment(mapper->ppuas, NAMETABLE1, NT_SIZE, mapper->vram);
        as_add_segment(mapper->ppuas, NAMETABLE2, NT_SIZE, mapper->vram + NT_SIZE);
        as_add_segment(mapper->ppuas, NAMETABLE3, NT_SIZE, mapper->vram + NT_SIZE);
    }
}

static uint8_t *map(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    return target; // Banks are fixed for both PRG and CHR.
}

static void write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value) {
    // Do nothing.
}
