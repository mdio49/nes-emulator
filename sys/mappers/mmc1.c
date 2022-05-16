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

#define N_REGISTERS     4
#define MMC1_SR_RESET   0x10

#define CHR_BANK0       0x0000
#define CHR_BANK1       0x1000
#define CHR_BANK_SIZE   0x1000

#define PRG_RAM         0x6000
#define PRG_RAM_SIZE    0x2000
#define N_RAM_BANKS     4

#define PRG_BANK0       0x8000
#define PRG_BANK1       0xC000
#define PRG_BANK_SIZE   0x4000

struct mmc1_data {

    uint8_t     chr_mask;
    uint8_t     prg_ram_bank;

};

static mapper_t *init(void);

static void insert(mapper_t *mapper, prog_t *prog);
static void monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write);

static uint8_t *map_ram(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);
static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);

const mapper_t mmc1 = {
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
    mapper->banks = calloc(N_REGISTERS, sizeof(uint8_t));
    mapper->r8[0] = MMC1_SR_RESET;

    /* additional data */
    mapper->data = malloc(sizeof(struct mmc1_data));
    ((struct mmc1_data*)mapper->data)->prg_ram_bank = 0;

    // Some programs expect first bank at $8000 and last bank at $C000 for PRG-ROM, and all 8KB of CHR
    // memory mapped to $0000, so the mapper will predefine this to support this behaviour.
    mapper->banks[0] = 0x0C;
    mapper->banks[1] = 0x00;
    mapper->banks[2] = 0x00;
    mapper->banks[3] = 0x00;
    
    return mapper;
}

static void insert(mapper_t *mapper, prog_t *prog) {
    // Switchable 8KB of PRG-RAM (if used; max 32KB).
    prog->prg_ram = malloc(N_RAM_BANKS * PRG_RAM_SIZE * sizeof(uint8_t));
    as_add_segment(mapper->cpuas, PRG_RAM, PRG_RAM_SIZE, prog->prg_ram, AS_READ | AS_WRITE);
    
    // Map each PRG bank to the start of the PRG-ROM.
    as_add_segment(mapper->cpuas, PRG_BANK0, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ);
    as_add_segment(mapper->cpuas, PRG_BANK1, PRG_BANK_SIZE, (uint8_t*)prog->prg_rom, AS_READ);

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

    // Determine mask used to determine CHR bank number (cached for efficiency).
    uint8_t mask;
    if (prog->header.chr_rom_size < 2)
        mask = 0x01;
    else if (prog->header.chr_rom_size < 4)
        mask = 0x03;
    else if (prog->header.chr_rom_size < 8)
        mask = 0x07;
    else if (prog->header.chr_rom_size < 16)
        mask = 0x0F;
    else
        mask = 0x1F;
    ((struct mmc1_data*)mapper->data)->chr_mask = mask;
}

static void monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write) {
    if (!write)
        return;
    if (as != mapper->cpuas)
        return;
    if (vaddr < PRG_ROM_START)
        return;
    
    if ((value & 0x80) > 0) {
        // Reset shift register.
        mapper->r8[0] = MMC1_SR_RESET;
    }
    else {
        // Check if the shift register is full.
        bool full = (mapper->r8[0] & 0x01) > 0;
        
        // Shift bit 0 into the register.
        mapper->r8[0] = ((value & 0x01) << 4) | (mapper->r8[0] >> 1);

        // If the shift register is full, then move the contents of the register into the
        // bank register and reset the shift register.
        if (full) {
            uint8_t bank = (vaddr >> 13) & 0x03;
            if (bank == 3) {
                mapper->banks[bank] = (mapper->banks[bank] & 0x10) | (mapper->r8[0] & 0x0F);
            }
            else {
                mapper->banks[bank] = mapper->r8[0];
            }
            mapper->r8[0] = MMC1_SR_RESET;

            // Update PRG-RAM bank if necessary.
            if (bank == 1 || bank == 2) {
                struct mmc1_data *data = (struct mmc1_data*)mapper->data;
                const uint8_t chr_mode = (mapper->banks[0] >> 2) & 0x03;
                if (bank == 1 || chr_mode == 1) {
                    // Only swap PRG-RAM if the lines are not being used for CHR memory.
                    if (prog->header.chr_rom_size < 2) {
                        data->prg_ram_bank = (mapper->banks[bank] >> 2) & 0x03;
                    }

                    // Also update bit 4 of PRG-ROM bank (if PRG-ROM is big enough).
                    if (prog->header.prg_rom_size == 32) {
                        mapper->banks[3] = (mapper->banks[bank] & 0x10) | (mapper->banks[3] & 0x0F);
                    }
                }
            }
        }
    }
}

static uint8_t *map_ram(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    struct mmc1_data *data = (struct mmc1_data*)mapper->data;
    const uint8_t bank = data->prg_ram_bank;
    return target + bank * PRG_RAM_SIZE;
}

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    const uint8_t mode = (mapper->banks[0] >> 2) & 0x03;
    const uint8_t bank = mapper->banks[3] & 0x0F;

    if (mode == 2) {
        // Fix first bank, switch second bank.
        if (vaddr >= PRG_BANK1) {
            target += bank * PRG_BANK_SIZE;
        }
    }
    else if (mode == 3) {
        // Fix second bank, switch first bank.
        if (vaddr < PRG_BANK1) {
            target += bank * PRG_BANK_SIZE;
        }
        else {
            target += ((N_PRG_BANKS(prog, PRG_BANK_SIZE) - 1) & 0x0F) * PRG_BANK_SIZE;
        }
    }
    else {
        // Switch entire 32KB bank (ignore lower bit of bank number).
        uint8_t *start = target - offset;
        offset = vaddr - PRG_BANK0; // Both banks are needed for offset.
        target = start + (bank & 0x0E) * PRG_BANK_SIZE + offset;
    }

    return target + (mapper->banks[3] & 0x10) * PRG_BANK_SIZE;
}

static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    struct mmc1_data *data = (struct mmc1_data*)mapper->data;
    const uint8_t mask = data->chr_mask;

    // Switch bank depending on mode.
    uint8_t mode = (mapper->banks[0] >> 4) & 0x01;
    if (mode == 1) {
        // Two separate 4KB banks.
        uint8_t bank = (vaddr >= CHR_BANK1 ? mapper->banks[2] : mapper->banks[1]) & mask;
        target += bank * CHR_BANK_SIZE;
    }
    else {
        // One combined 8KB bank.
        uint8_t *start = target - offset;
        offset = vaddr - CHR_BANK0; // Both banks are needed for offset.
        target = start + (mapper->banks[1] & ~0x01) * CHR_BANK_SIZE + offset;
    }

    return target;
}

static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    const bool mirror = (mapper->banks[0] & 0x02) > 0;
    const bool hz = (mapper->banks[0] & 0x01) > 0;
    const uint8_t nt = NT(vaddr);

    if (!mirror) {
        // One-screen (upper bank if bit 0 is set).
        if (hz) {
            target += NT_SIZE;
        }
    }
    else if (hz) {
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
