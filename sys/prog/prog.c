#include <prog.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void load_ines(prog_t *prog, const char *src) {
    // Get PRG-ROM and CHR-ROM size.
    prog->header.prg_rom_size = (uint8_t)src[4];
    prog->header.chr_rom_size = (uint8_t)src[5];

    // Decode flags 6.
    ines_flag6_t flag6 = { .bits = (uint8_t)src[6] };
    prog->header.four_screen = flag6.flags.four_screen;
    prog->header.mirroring = flag6.flags.mirroring;
    prog->header.prg_ram = flag6.flags.prg_ram;
    prog->header.trainer = flag6.flags.trainer;

    // Decode flags 7.
    ines_flag7_t flag7 = { .bits = (uint8_t)src[7] };
    prog->header.format = flag7.flags.format;
    prog->header.playchoice_10 = flag7.flags.playchoice_10;
    prog->header.vs_unisys = flag7.flags.vs_unisys;

    // Get mapper number.
    prog->header.mapper_no = (flag7.flags.mapper_high << 4) + flag6.flags.mapper_low;
    if (prog->header.mapper_no != 0) {
        printf("Emulator does not support mapper number %d.\n", prog->header.mapper_no);
        exit(1);
    }

    // Decode remaining flags depending on format.
    ines_flag8_t flag8 = { .bits = (uint8_t)src[8] };
    //ines_flag9_t flag9 = { .bits = (uint8_t)src[9] };
    if (prog->header.format == 2) {
        printf("Emulator does not support NES 2.0 file format.\n");
        exit(1);
    }
    else {
        prog->header.prg_ram_size = flag8.flags.prg_ram_size;
    }

    // Prepare copying of data.
    const char *fp = src + INES_HEADER_SIZE;
    const size_t prg_rom_bytes = prog->header.prg_rom_size * INES_PRG_ROM_UNIT;
    const size_t chr_rom_bytes = prog->header.chr_rom_size * INES_CHR_ROM_UNIT;
    
    // Copy trainer.
    if (prog->header.trainer) {
        prog->trainer = malloc(INES_TRAINER_SIZE);
        memcpy((void *)prog->trainer, fp, INES_TRAINER_SIZE);
        fp += INES_TRAINER_SIZE;
    }
    else {
        prog->trainer = NULL;
    }

    // Copy PRG-ROM.
    prog->prg_rom = malloc(prg_rom_bytes);
    memcpy((void *)prog->prg_rom, fp, prg_rom_bytes);
    fp += prg_rom_bytes;

    // Copy CHR-ROM.
    if (prog->header.chr_rom_size > 0) {
        prog->chr_rom = malloc(chr_rom_bytes);
        memcpy((void *)prog->chr_rom, fp, chr_rom_bytes);
        fp += chr_rom_bytes;
    }
    else {
        prog->chr_rom = NULL;
    }

    // Copy INST-ROM.
    if (prog->header.playchoice_10) {
        prog->inst_rom = malloc(INES_INST_ROM_SIZE);
        memcpy((void *)prog->inst_rom, fp, INES_INST_ROM_SIZE);
        fp += INES_INST_ROM_SIZE;
    }
    else {
        prog->inst_rom = NULL;
    }

    // Don't worry about PROM for now.
    prog->prom = NULL;
}

prog_t *prog_create(const char *src) {
    // Create the program struct.
    prog_t *prog = malloc(sizeof(struct prog));
    if (prog == NULL)
        return NULL;
    
    // Check first 4 bytes.
    bool ines_format = true;
    if (src[0] != 'N')
        ines_format = false;
    if (src[1] != 'E')
        ines_format = false;
    if (src[2] != 'S')
        ines_format = false;
    if (src[3] != 0x1A)
        ines_format = false;

    // Load depending on the format.
    if (ines_format) {
        load_ines(prog, src);
    }
    else {
        printf("Invalid file format.\n");
        exit(1);
    }

    return prog;
}

void prog_destroy(prog_t *prog) {
    if (prog->chr_rom != NULL)
        free((void*)prog->chr_rom);
    if (prog->prg_rom != NULL)
        free((void*)prog->prg_rom);
    if (prog->inst_rom != NULL)
        free((void*)prog->prg_rom);
    if (prog->trainer != NULL)
        free((void*)prog->trainer);
    if (prog->prom != NULL)
        free((void*)prog->prom);
    free(prog);
}

void prog_execute(prog_t *prog, cpu_t *cpu, ppu_t *ppu) {
    uint8_t prg_ram[0x2000];

    // TODO: Make the mapper responsible for this.
    //  |
    //  |
    // \|/
     
    // Setup CPU address space.
    as_add_segment(cpu->as, 0x6000, 0x2000, prg_ram);
    as_add_segment(cpu->as, 0x8000, 0x4000, (uint8_t*)prog->prg_rom);
    as_add_segment(cpu->as, 0xC000, 0x4000, (uint8_t*)prog->prg_rom + 0x4000);

    // Setup PPU address space.
    as_add_segment(ppu->as, 0x0000, 0x2000, (uint8_t*)prog->chr_rom);
    as_add_segment(ppu->as, 0x2000, 0x0400, ppu->vram);
    as_add_segment(ppu->as, 0x3000, 0x0400, ppu->vram);
    if (prog->header.mirroring == 1) {
        // Vertical mirroring.
        as_add_segment(ppu->as, 0x2400, 0x0400, ppu->vram + 0x0400);
        as_add_segment(ppu->as, 0x2800, 0x0400, ppu->vram);
        as_add_segment(ppu->as, 0x2C00, 0x0400, ppu->vram + 0x0400);

        as_add_segment(ppu->as, 0x3400, 0x0400, ppu->vram + 0x0400);
        as_add_segment(ppu->as, 0x3800, 0x0400, ppu->vram);
        as_add_segment(ppu->as, 0x3C00, 0x0300, ppu->vram + 0x0400);
    }
    else {
        // Horizontal mirroring.
        as_add_segment(ppu->as, 0x2400, 0x0400, ppu->vram);
        as_add_segment(ppu->as, 0x2800, 0x0400, ppu->vram + 0x0400);
        as_add_segment(ppu->as, 0x2C00, 0x0400, ppu->vram + 0x0400);

        as_add_segment(ppu->as, 0x3400, 0x0400, ppu->vram);
        as_add_segment(ppu->as, 0x3800, 0x0400, ppu->vram + 0x0400);
        as_add_segment(ppu->as, 0x3C00, 0x0300, ppu->vram + 0x0400);
    }

    // Palette memory.
    for (int i = 0; i < 8; i++) {
        addr_t offset = 0x3F00 + (i * 0x0020);
        as_add_segment(ppu->as, offset, 1, &ppu->bkg_color);
        for (int j = 0; j < N_PALETTES; j++) {
            as_add_segment(ppu->as, offset + 1 + j * 4, 3, (uint8_t*)&ppu->palettes[j]);
            as_add_segment(ppu->as, offset + (j + 1) * 4, 1, &ppu->bkg_color);
        }
    }

    // Execute program (loops forever unless interrupted).
    cpu_reset(cpu);
    while (true) {
        // Execute the next instruction.
        uint8_t *insptr = cpu_fetch(cpu);
        operation_t ins = cpu_decode(cpu, insptr);
        cpu_execute(cpu, ins);
    }
}
