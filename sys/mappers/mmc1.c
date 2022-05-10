#include <mappers.h>
#include <stdio.h>
#include <stdlib.h>
#include <ppu.h>

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

static mapper_t *init(void);

static void insert(mapper_t *mapper, prog_t *prog);
static void write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value);

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *start, size_t offset);
static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *start, size_t offset);
static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *start, size_t offset);

const mapper_t mmc1 = {
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
    mapper->sr[0] = MMC1_SR_RESET;
    
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

    //mapper->banks[0] = 0x0C;
}

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    uint8_t mode = (mapper->banks[0] >> 2) & 0x03;
    uint8_t bank = mapper->banks[3] & 0x0F;

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
            target += (prog->header.prg_rom_size - 1) * PRG_BANK_SIZE;
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
    uint8_t mode = (mapper->banks[0] >> 4) & 0x01;
    if (mode == 1) {
        // Two separate 4KB banks.
        uint8_t bank = vaddr >= CHR_BANK1 ? mapper->banks[2] : mapper->banks[1];
        target += bank * CHR_BANK_SIZE;
    }
    else {
        // One combined 8KB bank.
        uint8_t *start = target - offset;
        offset = vaddr - CHR_BANK0; // Both banks are needed for offset.
        target = start + (mapper->banks[1] & 0x1E) * CHR_BANK_SIZE + offset;
    }
    return target;
}

static uint8_t *map_nts(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    bool mirror = (mapper->banks[0] & 0x02) > 0;
    bool hz = (mapper->banks[0] & 0x01) > 0;
    uint8_t nt = NT(vaddr);

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

static void write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value) {
    if (vaddr < PRG_ROM_START)
        return;
    
    if ((value & 0x70) > 0) {
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
        }
    }
}
