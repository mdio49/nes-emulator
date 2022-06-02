#include <mappers.h>
#include <stdio.h>
#include <stdlib.h>
#include <cpu.h>
#include <ppu.h>

#define PRG_MODE        0x5100
#define CHR_MODE        0x5101
#define PRG_RAM_PRTC1   0x5102
#define PRG_RAM_PRTC2   0x5103
#define EX_RAM_MODE     0x5104
#define NT_MAPPING      0x5105
#define FILL_MODE_TILE  0x5106
#define FILL_MODE_COLOR 0x5107

#define PRG_SELECT      0x5113
#define CHR_SELECT      0x5120

#define V_SPLIT_MODE    0x5201
#define V_SPLIT_SCROLL  0x5201
#define V_SPLIT_BANK    0x5202

#define IRQ_COMPARE     0x5203
#define IRQ_STATUS      0x5204

#define MULT_LOW        0x5205
#define MULT_HIGH       0x5206

#define CHR_BANK0       0x0000
#define CHR_BANK_SIZE   0x0400

#define PRG_RAM         0x6000
#define PRG_RAM_SIZE    0x20000

#define PRG_BANK0       0x8000
#define PRG_BANK1       0xA000
#define PRG_BANK2       0xC000
#define PRG_BANK3       0xE000
#define PRG_BANK_SIZE   0x2000

#define IN_FRAME_MASK   0x40
#define IRQ_ACK_MASK    0x80

struct mmc5_data {

    uint8_t     prg_mode;           // PRG mode.
    uint8_t     chr_mode;           // CHR mode.

    uint8_t     prg_ram_protect_1;  // PRG-RAM protect 1.
    uint8_t     prg_ram_protect_2;  // PRG-RAM protect 2.

    uint8_t     ex_ram_mode;        // EX-RAM mode.
    uint8_t     nt_mapping;         // Nametable mapping.

    uint8_t     fill_mode_tile;     // Tile used for fill mode.
    uint8_t     fill_mode_color;    // Attribute used for fill mode.

    struct multiplier {             // Multiplier circuit (for unsigned 8-bit multiplication).
        uint8_t     in1, in2;       // Two unsigned 8-bit inputs.
        uint8_t     out_low;        // Low byte of unsigned 16-bit result.
        uint8_t     out_high;       // High byte of unsigned 16-bit result.
    } multiplier;

    uint8_t     prg_banks[5];       // PRG bank select.
    uint8_t     chr_banks[12];      // CHR bank select.
    uint8_t     ex_ram[1024];       // 1KB of on-chip memory.

    uint8_t     v_split_mode;       // Vertical split mode.
    uint8_t     v_split_scroll;     // Vertical split scroll.
    uint8_t     v_split_bank;       // Vertical split bank.

    uint8_t     irq_scanline;       // IRQ scanline compare value.
    uint8_t     irq_status;         // IRQ scanline compare value.

    uint8_t     prg_mask;           // PRG-ROM mask.
    uint8_t     chr_mask;           // CHR-ROM mask.
    uint8_t     read_buffer;        // Read buffer used for fill mode and NT mode 2 with no EX-RAM.

    unsigned    sprite_sz   : 1;    // Internal account of PPU sprite size (0: 8x8; 1: 16x16).
    unsigned    rendering   : 2;    // Internal account of whether the PPU is rendering (0: disabled; 1,2,3: enabled).
    unsigned    bkg_flag    : 1;    // Set if the PPU is currently rendering the background.
    unsigned    ppu_reading : 1;    // Set if the PPU is currently reading.
    unsigned    irq_enable  : 1;    // Set if scanline IRQ is enabled.
    unsigned                : 2;

    addr_t      last_ppu_addr;      // Last PPU address read from.
    uint8_t     match_count;
    uint8_t     idle_count;
    uint8_t     nt_bytes_read;
    uint8_t     scanline;           // Current scanline.

};

static mapper_t *init(void);

static void insert(mapper_t *mapper, prog_t *prog);
static void monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write);
static void cycle(mapper_t *mapper, prog_t *prog, int cycles);
static float mix(mapper_t *mapper, prog_t *prog, float input);

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
    mapper->cycle = cycle;
    mapper->mix = mix;

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

    as_add_segment(mapper->cpuas, V_SPLIT_MODE, 1, &data->v_split_mode, AS_WRITE);
    as_add_segment(mapper->cpuas, V_SPLIT_SCROLL, 1, &data->v_split_scroll, AS_WRITE);
    as_add_segment(mapper->cpuas, V_SPLIT_BANK, 1, &data->v_split_bank, AS_WRITE);

    as_add_segment(mapper->cpuas, IRQ_COMPARE, 1, &data->irq_scanline, AS_WRITE);
    as_add_segment(mapper->cpuas, IRQ_STATUS, 1, &data->irq_status, AS_READ);

    as_add_segment(mapper->cpuas, MULT_LOW, 1, &data->multiplier.out_low, AS_READ);
    as_add_segment(mapper->cpuas, MULT_HIGH, 1, &data->multiplier.out_high, AS_READ);
    
    // Switchable 8KB of PRG-RAM (128KB allocated).
    prog->prg_ram = malloc(PRG_RAM_SIZE * sizeof(uint8_t));
    as_add_segment(mapper->cpuas, PRG_RAM, PRG_BANK_SIZE, prog->prg_ram, AS_READ | AS_WRITE);
    
    // Up to 4 switchable 8KB PRG-ROM (and RAM) banks (map it to start of PRG-ROM).
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

    // Games expect $5017 = $FF at power on.
    data->prg_banks[4] = 0xFF;

    // IRQ registers power-on values.
    data->scanline = 0;
    data->irq_status = 0x00;

    // Multiplication registers power-on values.
    data->multiplier.in1 = 0xFF;
    data->multiplier.in2 = 0xFF;
    data->multiplier.out_low = 0x01;
    data->multiplier.out_high = 0xFE;

    // Determine mask used to determine PRG and CHR bank numbers.
    uint8_t mask;
    if (prog->header.prg_rom_size < 2)
        mask = 0x01;
    else if (prog->header.prg_rom_size < 4)
        mask = 0x03;
    else if (prog->header.prg_rom_size < 8)
        mask = 0x07;
    else if (prog->header.prg_rom_size < 16)
        mask = 0x0F;
    else if (prog->header.prg_rom_size < 32)
        mask = 0x1F;
    else if (prog->header.prg_rom_size < 64)
        mask = 0x3F;
    else
        mask = 0x7F;
    data->prg_mask = mask;

    if (prog->header.chr_rom_size < 4)
        mask = 0x01;
    else if (prog->header.chr_rom_size < 8)
        mask = 0x03;
    else if (prog->header.chr_rom_size < 16)
        mask = 0x07;
    else if (prog->header.chr_rom_size < 32)
        mask = 0x0F;
    else if (prog->header.chr_rom_size < 64)
        mask = 0x1F;
    else if (prog->header.chr_rom_size < 128)
        mask = 0x3F;
    else
        mask = 0x7F;
    data->chr_mask = mask;
}

static void monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write) {
    struct mmc5_data *data = (struct mmc5_data*)mapper->data;
    if (as == mapper->cpuas) {
        // CPU
        if (write) {
            if (vaddr == PPU_CTRL) {
                // Update sprite size flag.
                data->sprite_sz = (value >> 5) & 0x01;
            }
            else if (vaddr == PPU_MASK) {
                // Determine whether rendering is enabled.
                data->rendering = (value >> 3) & 0x03;
                if (!data->rendering) {
                    // Clear in-frame flag if not rendering.
                    data->irq_status = data->irq_status & ~IN_FRAME_MASK;
                }
            }
            else if (vaddr == IRQ_STATUS) {
                // Set scanline IRQ enable flag.
                data->irq_enable = (value & 0x80) > 0;
            }
            else if (vaddr == MULT_LOW || vaddr == MULT_HIGH) {
                // Determine which register is being written to.
                if (vaddr == MULT_LOW) {
                    data->multiplier.in1 = value;
                }
                else {
                    data->multiplier.in2 = value;
                }

                // Computer the unsigned 16-bit product.
                uint16_t result = data->multiplier.in1 * data->multiplier.in2;

                // Store the result in the registers so that it may be read.
                data->multiplier.out_low = result & 0xFF;
                data->multiplier.out_high = result >> 8;
            }
        }
        else if (vaddr == IRQ_STATUS) {
            // IRQ is acknowledged when IRQ status register is read.
            data->irq_enable = data->irq_enable & ~IRQ_ACK_MASK;
        }
        else if (vaddr == NMI_VECTOR || vaddr == NMI_VECTOR + 1) {
            // Scanline counter, IRQ status and in-frame flag is reset on NMI.
            data->irq_status = 0x00;
            data->scanline = 0;
        }
        else if (vaddr == RES_VECTOR || vaddr == RES_VECTOR + 1) {
            // Scanline IRQ is disabled on reset.
            data->irq_enable = false;
        }
    }
    else if (!write) {
        // Detect when the PPU makes 3 consecutive reads from the same nametable address.
        if (vaddr >= NAMETABLE0 && vaddr < NAMETABLE3 + NT_SIZE && vaddr == data->last_ppu_addr) {
            data->match_count++;
            if (data->match_count == 2) {
                // Start of scanline.
                if ((data->irq_status & IN_FRAME_MASK) == 0) {
                    // Set in-frame flag and reset scanline counter.
                    data->irq_status |= IN_FRAME_MASK;
                    data->scanline = 0;
                }
                else {
                    // Increment scanline counter.
                    data->scanline++;

                    // Determine whether the IRQ flag should be set.
                    if (data->scanline == data->irq_scanline) {
                        data->irq_status |= IRQ_ACK_MASK;
                        if (data->irq_enable) {
                            mapper->irq = true;
                        }
                    }
                }

                // Reset background flag and NT byte counter.
                data->bkg_flag = true;   
                data->nt_bytes_read = 0;
            }
        }
        else {
            data->match_count = 0;
        }
        data->last_ppu_addr = vaddr;
        data->ppu_reading = true;
        
        // Detect when the PPU stops reading the background.
        if (vaddr < NAMETABLE0) {
            data->nt_bytes_read++;
            if (data->nt_bytes_read == 64) {
                data->bkg_flag = false;
            }
            if (data->nt_bytes_read == 80) {
                data->bkg_flag = true;
            }
        }
    }
}

static void cycle(mapper_t *mapper, prog_t *prog, int cycles) {
    struct mmc5_data *data = (struct mmc5_data*)mapper->data;
    if (data->ppu_reading) {
        data->idle_count = 0;
    }
    else if (data->idle_count >= 3) {
        // Clear in-frame flag.
        data->irq_status = data->irq_status & ~IN_FRAME_MASK;
    }
    else {
        data->idle_count += cycles;
    }
    data->ppu_reading = false;
}

static float mix(mapper_t *mapper, prog_t *prog, float input) {
    // TODO
    return input;
}

static uint8_t *map_ram(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    struct mmc5_data *data = (struct mmc5_data*)mapper->data;
    return target + (data->prg_banks[0] & 0x7F) * PRG_BANK_SIZE;
}

static uint8_t *map_prg(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    struct mmc5_data *data = (struct mmc5_data*)mapper->data;
    const uint8_t bank = (vaddr - PRG_BANK0) / PRG_BANK_SIZE;
    const uint8_t mode = data->prg_mode & 0x03;

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
    uint8_t mask = prg_ram_select ? 0x0F : data->prg_mask;
    return target + (select & mask) * PRG_BANK_SIZE;
}

static uint8_t *map_chr(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    struct mmc5_data *data = (struct mmc5_data*)mapper->data;
    const uint8_t bank = (vaddr - CHR_BANK0) / CHR_BANK_SIZE;
    const uint8_t mode = data->chr_mode & 0x03;

    uint8_t mask, size;
    if (mode == 0) {
        // 8KB pages.
        size = 8;
        mask = 0x07;
    }
    else if (mode == 1) {
        // 4KB pages.
        size = 4;
        mask = 0x03;
    }
    else if (mode == 2) {
        // 2KB pages.
        size = 2;
        mask = 0x01;
    }
    else {
        // 1KB pages.
        size = 1;
        mask = 0x00;
    }
    
    // Offset based on the page that is selected for the current bank.
    uint8_t index;
    if (data->bkg_flag && data->sprite_sz) {
        index = 0x08 | (((bank & ~mask) | mask) & 0x03);
    }
    else {
        index = ((bank & ~mask) | mask) & 0x07;
    }
    target += data->chr_banks[index] * size * CHR_BANK_SIZE;

    // Adjust the address to the correct bank (as each 1KB segment points to the top of memory).
    target += (bank & mask) * CHR_BANK_SIZE;

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
        if (((vaddr >> 5) & 0x1F) < 30) {
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
