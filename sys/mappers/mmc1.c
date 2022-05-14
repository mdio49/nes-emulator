#include <mappers.h>
#include <stdio.h>
#include <stdlib.h>
#include <ppu.h>
#include <sys.h>

/**
 * bank registers:
 * 0 - control
 * 1 - CHR bank 0
 * 2 - CHR bank 1
 * 3 - PRG bank
 */

#define N_BANKS         4
#define MMC1_SR_RESET   0x10

#define CHR_BANK0       0x0000
#define CHR_BANK1       0x1000
#define CHR_BANK_SIZE   0x1000

#define PRG_RAM         0x6000
#define PRG_RAM_SIZE    0x2000

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
    mapper->banks = calloc(N_BANKS, sizeof(uint8_t));
    mapper->sr[0] = MMC1_SR_RESET;

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
    // Fixed 8KB of PRG-RAM (may not be used).
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
    if (prog->chr_sz < 16 * 1024)
        mask = 0x01;
    else if (prog->chr_sz < 32 * 1024)
        mask = 0x03;
    else if (prog->chr_sz < 64 * 1024)
        mask = 0x07;
    else if (prog->chr_sz < 128 * 1024)
        mask = 0x0F;
    else
        mask = 0x1F;
    ((struct mmc1_data*)mapper->data)->chr_mask = mask;

    //printf("PRG-ROM size: %dK\n", prog->prg_rom_sz / 1024);
    //printf("PRG-RAM size: %dK\n", prog->prg_ram_sz / 1024);
    //printf("CHR size: %dK\n", prog->chr_sz / 1024);
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
        mapper->sr[0] = MMC1_SR_RESET;
    }
    else {
        // Check if the shift register is full.
        bool full = (mapper->sr[0] & 0x01) > 0;
        
        // Shift bit 0 into the register.
        mapper->sr[0] = ((value & 0x01) << 4) | (mapper->sr[0] >> 1);

        // If the shift register is full, then move the contents of the register into the
        // bank register and reset the shift register.
        if (full) {
            uint8_t bank = (vaddr >> 13) & 0x03;
            mapper->banks[bank] = mapper->sr[0];
            mapper->sr[0] = MMC1_SR_RESET;

            //printf("%lld: register %d = $%.2x\n", cpu->cycles, bank, mapper->banks[bank]);

            // Update PRG-RAM bank if necessary.
            if (bank == 1 || bank == 2) {
                struct mmc1_data *data = (struct mmc1_data*)mapper->data;
                const uint8_t chr_mode = (mapper->banks[0] >> 2) & 0x03;
                if (bank == 1 || chr_mode == 1) {
                    data->prg_ram_bank = (mapper->banks[bank] >> 2) & 0x03;
                    //printf("%d\n", data->prg_ram_bank);

                    // Also update bit 4 of PRG-ROM bank (if PRG-ROM is big enough).
                    if (prog->prg_rom_sz == 512 * 1024) {
                        mapper->banks[3] = (mapper->banks[bank] & 0x10) | (mapper->banks[3] & 0x0F);
                    }
                }
            }
        }    }
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
            const int nbanks = prog->prg_rom_sz / PRG_BANK_SIZE;
            target += (nbanks - 1) * PRG_BANK_SIZE;
        }
    }
    else {
        // Switch entire 32KB bank (ignore lower bit of bank number).
        uint8_t *start = target - offset;
        offset = vaddr - PRG_BANK0; // Both banks are needed for offset.
        target = start + (bank & 0x0E) * PRG_BANK_SIZE + offset;
    }

    return target;
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
