#include <mappers.h>
#include <stdio.h>
#include <stdlib.h>
#include <ppu.h>

/**
 * bank registers:
 * 0 - PRG-ROM bank
 * 1 - CHR bank 0/$FD
 * 2 - CHR bank 0/$FE
 * 3 - CHR bank 1/$FD
 * 4 - CHR bank 1/$FE
 * 5 - mirroring
 */

#define N_REGISTERS     6

#define CHR_BANK0       0x0000
#define CHR_BANK1       0x1000
#define CHR_BANK_SIZE   0x1000

#define PRG_RAM         0x6000
#define PRG_RAM_SIZE    0x2000

#define PRG_BANK0       0x8000
#define PRG_BANK1       0xA000
#define PRG_BANK2       0xC000
#define PRG_BANK3       0xE000
#define PRG_BANK_SIZE   0x2000

static mapper_t *init(void);

static void insert(mapper_t *mapper, prog_t *prog);
static void monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write);

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);

const mapper_t mmc2 = {
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
    mapper->map_nts = map_nts;
    
    /* setup registers */
    mapper->banks = calloc(N_REGISTERS, sizeof(uint8_t));
    mapper->r8[0] = 0xFD; // latch 0
    mapper->r8[1] = 0xFD; // latch 1
    
    return mapper;
}

static void insert(mapper_t *mapper, prog_t *prog) {
    // Fixed 8KB of PRG-RAM (if used).
    prog->prg_ram = malloc(PRG_RAM_SIZE * sizeof(uint8_t));
    as_add_segment(mapper->cpuas, PRG_RAM, PRG_RAM_SIZE, prog->prg_ram, AS_READ | AS_WRITE);
    
    // Single 8KB switchable PRG-ROM bank.
    as_add_segment(mapper->cpuas, PRG_BANK0, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ);

    // Three 8KB PRG-ROM banks fixed to the last 3 banks.
    as_add_segment(mapper->cpuas, PRG_BANK1, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom + ((N_PRG_BANKS(prog, PRG_BANK_SIZE) - 3) * PRG_BANK_SIZE), AS_READ);
    as_add_segment(mapper->cpuas, PRG_BANK2, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom + ((N_PRG_BANKS(prog, PRG_BANK_SIZE) - 2) * PRG_BANK_SIZE), AS_READ);
    as_add_segment(mapper->cpuas, PRG_BANK3, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom + ((N_PRG_BANKS(prog, PRG_BANK_SIZE) - 1) * PRG_BANK_SIZE), AS_READ);

    // Map each CHR bank to the start of the CHR-ROM (or CHR-RAM if it is used).
    if (prog->chr_rom != NULL) {
        as_add_segment(mapper->ppuas, CHR_BANK0, CHR_BANK_SIZE, (uint8_t*)prog->chr_rom, AS_READ);
        as_add_segment(mapper->ppuas, CHR_BANK1, CHR_BANK_SIZE, (uint8_t*)prog->chr_rom, AS_READ);
    }
    else {
        as_add_segment(mapper->ppuas, CHR_BANK0, CHR_BANK_SIZE, prog->chr_ram, AS_READ | AS_WRITE);
        as_add_segment(mapper->ppuas, CHR_BANK1, CHR_BANK_SIZE, prog->chr_ram, AS_READ | AS_WRITE);
    }

    // Map each nametable to the start of VRAM.
    as_add_segment(mapper->ppuas, NAMETABLE0, NT_SIZE, mapper->vram, AS_READ | AS_WRITE);
    as_add_segment(mapper->ppuas, NAMETABLE1, NT_SIZE, mapper->vram, AS_READ | AS_WRITE);
    as_add_segment(mapper->ppuas, NAMETABLE2, NT_SIZE, mapper->vram, AS_READ | AS_WRITE); 
    as_add_segment(mapper->ppuas, NAMETABLE3, NT_SIZE, mapper->vram, AS_READ | AS_WRITE);
}

static void monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write) {
    if (write && as == mapper->cpuas && vaddr >= 0xA000) {
        // Update the appropriate bank register.
        mapper->banks[(vaddr >> 12) - 0x0A] = value;
    }
    else if (!write && as == mapper->ppuas && vaddr < NAMETABLE0) {
        // Update the value of the appropriate latch (if required).
        uint8_t pt = (vaddr >> 12) & 0x01;
        uint8_t tile = (vaddr >> 4) & 0xFF;
        if (tile == 0xFD || tile == 0xFE) {
            mapper->r8[pt] = tile;
        }
    }
}

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    // Only the first bank is switchable.
    if (vaddr < PRG_BANK1) {
        target += (mapper->banks[0] & 0x0F) * PRG_BANK_SIZE;
    }

    return target;
}

static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    uint8_t pt = (vaddr >> 12) & 0x01;
    uint8_t reg = 1 + 2 * pt + (mapper->r8[pt] - 0xFD);
    target += (mapper->banks[reg] & 0x1F) * CHR_BANK_SIZE;
    return target;
}

static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    uint8_t nt = NT(vaddr);
    if ((mapper->banks[5] & 0x01) > 0) {
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
