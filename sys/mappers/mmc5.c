#include <mappers.h>
#include <stdio.h>
#include <stdlib.h>
#include <ppu.h>

#define PRG_MODE        0x5000
#define CHR_MODE        0x5001
#define PRG_RAM_PRTC1   0x5002
#define PRG_RAM_PRTC2   0x5003
#define EX_RAM_MODE     0x5004
#define NT_MAPPING      0x5005
#define FILL_MODE_TILE  0x5006
#define FILL_MODE_COLOR 0x5007

#define CHR_BANK0       0x0000
#define CHR_BANK_SIZE   0x0400

#define PRG_RAM         0x6000
#define PRG_RAM_SIZE    0x20000

#define PRG_BANK0       0x8000
#define PRG_BANK1       0xA000
#define PRG_BANK2       0xC000
#define PRG_BANK3       0xE000
#define PRG_BANK_SIZE   0x2000

struct mmc5_data {

    uint8_t     prg_mode;
    uint8_t     chr_mode;

    uint8_t     prg_ram_protect_1;
    uint8_t     prg_ram_protect_2;

    uint8_t     ex_ram_mode;
    uint8_t     nt_mapping;

    uint8_t     fill_mode_tile;
    uint8_t     fill_mode_color;

};

static mapper_t *init(void);

static void insert(mapper_t *mapper, prog_t *prog);
static void monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write);

static uint8_t *map_ram(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);

const mapper_t mmc5 = {
    .init = init
};

static mapper_t *init(void) {
    /* create mapper */
    mapper_t *mapper = mapper_create();

    /* set functions */
    mapper->insert = insert;
    mapper->monitor = monitor;

    /* set mapper rules */
    mapper->map_ram = map_ram;
    mapper->map_prg = map_prg;
    mapper->map_chr = map_chr;
    mapper->map_nts = map_nts;
    
    /* setup registers */
    mapper->banks = NULL; // MMC5 uses memory-mapped registers in $5000-5FFF region.

    /* additional data */
    mapper->data = malloc(sizeof(struct mmc5_data));
    
    return mapper;
}

static void insert(mapper_t *mapper, prog_t *prog) {
    struct mmc5_data *data = (struct mmc5_data*)mapper->data;

    // Map registers.
    as_add_segment(mapper->cpuas, PRG_MODE, 1, &data->prg_mode, AS_WRITE);
    as_add_segment(mapper->cpuas, CHR_MODE, 1, &data->chr_mode, AS_WRITE);
    as_add_segment(mapper->cpuas, PRG_RAM_PRTC1, 1, &data->prg_ram_protect_1, AS_WRITE);
    as_add_segment(mapper->cpuas, PRG_RAM_PRTC2, 1, &data->prg_ram_protect_2, AS_WRITE);
    as_add_segment(mapper->cpuas, EX_RAM_MODE, 1, &data->ex_ram_mode, AS_WRITE);
    as_add_segment(mapper->cpuas, NT_MAPPING, 1, &data->nt_mapping, AS_WRITE);
    as_add_segment(mapper->cpuas, FILL_MODE_TILE, 1, &data->fill_mode_tile, AS_WRITE);
    as_add_segment(mapper->cpuas, FILL_MODE_COLOR, 1, &data->fill_mode_color, AS_WRITE);
    
    // Switchable 8KB of PRG-RAM (128KB allocated).
    prog->prg_ram = malloc(PRG_RAM_SIZE * sizeof(uint8_t));
    as_add_segment(mapper->cpuas, PRG_RAM, PRG_RAM_SIZE, prog->prg_ram, AS_READ | AS_WRITE);
    
    // Up to 4 switchable PRG-ROM (and RAM) banks (map it to start of PRG-ROM).
    as_add_segment(mapper->cpuas, PRG_BANK0, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ | AS_WRITE);
    as_add_segment(mapper->cpuas, PRG_BANK1, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ | AS_WRITE);
    as_add_segment(mapper->cpuas, PRG_BANK2, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ | AS_WRITE);

    // Last bank is always PRG-ROM (so make it read only).
    as_add_segment(mapper->cpuas, PRG_BANK3, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ);

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

static void monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write) {
    // TODO
}

static uint8_t *map_ram(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    // TODO
    return target;
}

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    // TODO
    return target;
}

static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    // TODO
    return target;
}

static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    struct mmc5_data *data = (struct mmc5_data*)mapper->data;
    uint8_t shift = NT(vaddr) * 2;
    uint8_t mask = 0x03 << shift;
    uint8_t nt = (data->nt_mapping & mask) >> shift;
    return target + nt * NT_SIZE; // TODO: Not exactly how it should work if nt = 3 or 4.
}
