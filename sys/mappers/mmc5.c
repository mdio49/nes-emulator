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

#define PRG_SELECT      0x5013
#define CHR_SELECT      0x5020

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

    uint8_t     prg_mode;           // PRG mode
    uint8_t     chr_mode;           // CHR mode

    uint8_t     prg_ram_protect_1;  // PRG-RAM protect 1
    uint8_t     prg_ram_protect_2;  // PRG-RAM protect 2

    uint8_t     ex_ram_mode;        // EX-RAM mode
    uint8_t     nt_mapping;         // Nametable mapping

    uint8_t     fill_mode_tile;     // Tile used for fill mode
    uint8_t     fill_mode_color;    // Attribute used for fill mode

    uint8_t     prg_banks[5];       // PRG bank select
    uint8_t     chr_banks[12];      // CHR bank select
    uint8_t     ex_ram[1024];       // 1KB of on-chip memory

    uint8_t     read_buffer;        // Read buffer used for fill mode and NT mode 2 with no EX-RAM.

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

    as_add_segment(mapper->cpuas, PRG_SELECT, sizeof(data->prg_banks), data->prg_banks, AS_WRITE);
    as_add_segment(mapper->cpuas, CHR_SELECT, sizeof(data->chr_banks), data->chr_banks, AS_WRITE);
    
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
    if (as == mapper->cpuas) {
        // CPU
        
    }
    else {
        // PPU

    }
}

static uint8_t *map_ram(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    struct mmc5_data *data = (struct mmc5_data*)mapper->data;
    return target + (data->prg_banks[0] & 0x7F) * PRG_BANK_SIZE;
}

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    struct mmc5_data *data = (struct mmc5_data*)mapper->data;
    const uint8_t bank = (vaddr - PRG_BANK0) / PRG_BANK_SIZE;
    const uint8_t mode = data->prg_mode;

    // Determine the bank selection and ROM/RAM select depending on the mode.
    uint8_t select;
    bool prg_ram_select = false;
    if (mode == 0) {
        // 32KB switchable ROM bank.
        select = (data->prg_banks[4] & 0x7C) | (bank & 0x03);
    }
    else if (mode == 1) {
        if (vaddr < PRG_BANK2) {
            // 16KB switchable ROM/RAM bank.
            prg_ram_select = ((data->prg_banks[2] & 0x80) == 0);
            select = (data->prg_banks[2] & 0x7E) | (bank & 0x01);
        }
        else {
            // 16KB switchable ROM bank.
            select = (data->prg_banks[4] & 0x7E) | (bank & 0x01);
        }
    }
    else if (mode == 2) {   
        if (vaddr < PRG_BANK2) {
            // 16KB switchable ROM/RAM bank.
            prg_ram_select = ((data->prg_banks[2] & 0x80) == 0);
            select = (data->prg_banks[2] & 0x7E) | (bank & 0x01);
        }
        else if (vaddr < PRG_BANK3) {
            // 8KB switchable ROM/RAM bank.
            prg_ram_select = ((data->prg_banks[3] & 0x80) == 0);
            select = data->prg_banks[3] & 0x7F;
        }
        else {
            // 8KB switchable ROM bank.
            select = data->prg_banks[4] & 0x7F;
        }
    }
    else {
        if (vaddr < PRG_BANK1) {
            // 8KB switchable ROM/RAM bank.
            prg_ram_select = ((data->prg_banks[1] & 0x80) == 0);
            select = data->prg_banks[1] & 0x7F;
        }
        else if (vaddr < PRG_BANK2) {
            // 8KB switchable ROM/RAM bank.
            prg_ram_select = ((data->prg_banks[2] & 0x80) == 0);
            select = data->prg_banks[2] & 0x7F;
        }
        else if (vaddr < PRG_BANK3) {
            // 8KB switchable ROM/RAM bank.
            prg_ram_select = ((data->prg_banks[3] & 0x80) == 0);
            select = data->prg_banks[3] & 0x7F;
        }
        else {
            // 8KB switchable ROM bank.
            select = data->prg_banks[4] & 0x7F;
        }
    }

    // Switch to PRG-RAM if RAM is selected.
    if (prg_ram_select) {
        target = prog->prg_ram + offset;
    }

    // Adjust the offset based on the selected bank.
    target += select * PRG_BANK_SIZE;

    return target;
}

static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    // TODO
    return target;
}

static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    struct mmc5_data *data = (struct mmc5_data*)mapper->data;
    const uint8_t shift = NT(vaddr) * 2;
    const uint8_t mask = 0x03 << shift;
    const uint8_t nt = (data->nt_mapping & mask) >> shift;
    if (nt < 2) {
        // Normal nametable behaviour.
        target += nt * NT_SIZE;
    }
    else if (nt == 4) {
        // Fill mode.
        if (((vaddr & 0xF0) >> 4) >= 15) {
            data->read_buffer = data->fill_mode_tile;
            target = &data->read_buffer;
        }
        else {
            uint8_t attr = data->fill_mode_color & 0x03;
            data->read_buffer = (attr << 6) | (attr << 4) | (attr << 2) | attr;
            target = &data->read_buffer;
        }
    }
    else if ((data->ex_ram_mode & 0x02) == 0) {
        // Use internal EX-RAM for nametable.
        target = data->ex_ram + offset;
    }
    else {
        // Data is read as zeroes.
        data->read_buffer = 0x00;
        target = &data->read_buffer;
    }
    return target;
}
