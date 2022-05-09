#include <mappers.h>
#include <stdlib.h>

static mapper_t *init(void);
static void insert(mapper_t *mapper, prog_t *prog);
static void write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value);

const mapper_t nrom = {
    .init = init
};

static mapper_t *init(void) {
    /* create mapper */
    mapper_t *mapper = malloc(sizeof(struct mapper));

    /* set functions */
    mapper->insert = insert;
    mapper->write = write;
    
    /* setup registers */
    mapper->banks = NULL;
    
    return mapper;
}

static void insert(mapper_t *mapper, prog_t *prog) {
    // Setup CPU address space.
    as_add_segment(mapper->cpuas, 0x6000, 0x2000, prog->prg_ram);
    as_add_segment(mapper->cpuas, 0x8000, 0x4000, (uint8_t*)prog->prg_rom);
    if (prog->header.prg_rom_size > 1) {
        as_add_segment(mapper->cpuas, 0xC000, 0x4000, (uint8_t*)prog->prg_rom + 0x4000);
    }
    else {
        as_add_segment(mapper->cpuas, 0xC000, 0x4000, (uint8_t*)prog->prg_rom);
    }

    // Setup PPU address space.
    for (int i = 0; i < 4; i++) {
        // Address space is mirrored every 0x4000 bytes.
        addr_t mirror = 0x4000 * i;
    
        // Pattern tables.
        if (prog->chr_rom != NULL) {
            as_add_segment(mapper->ppuas, mirror + 0x0000, 0x2000, (uint8_t*)prog->chr_rom);
        }
        else {
            as_add_segment(mapper->ppuas, mirror + 0x0000, 0x2000, prog->chr_ram);
        }

        // Name tables.
        as_add_segment(mapper->ppuas, mirror + 0x2000, 0x0400, mapper->vram);
        as_add_segment(mapper->ppuas, mirror + 0x3000, 0x0400, mapper->vram);
        if (prog->header.mirroring == 1) {
            // Vertical mirroring.
            as_add_segment(mapper->ppuas, mirror + 0x2400, 0x0400, mapper->vram + 0x0400);
            as_add_segment(mapper->ppuas, mirror + 0x2800, 0x0400, mapper->vram);
            as_add_segment(mapper->ppuas, mirror + 0x2C00, 0x0400, mapper->vram + 0x0400);

            as_add_segment(mapper->ppuas, mirror + 0x3400, 0x0400, mapper->vram + 0x0400);
            as_add_segment(mapper->ppuas, mirror + 0x3800, 0x0400, mapper->vram);
            as_add_segment(mapper->ppuas, mirror + 0x3C00, 0x0300, mapper->vram + 0x0400);
        }
        else {
            // Horizontal mirroring.
            as_add_segment(mapper->ppuas, mirror + 0x2400, 0x0400, mapper->vram);
            as_add_segment(mapper->ppuas, mirror + 0x2800, 0x0400, mapper->vram + 0x0400);
            as_add_segment(mapper->ppuas, mirror + 0x2C00, 0x0400, mapper->vram + 0x0400);

            as_add_segment(mapper->ppuas, mirror + 0x3400, 0x0400, mapper->vram);
            as_add_segment(mapper->ppuas, mirror + 0x3800, 0x0400, mapper->vram + 0x0400);
            as_add_segment(mapper->ppuas, mirror + 0x3C00, 0x0300, mapper->vram + 0x0400);
        }
    }
}

static void write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value) {
    // Do nothing.
}
