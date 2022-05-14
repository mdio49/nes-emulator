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
    prog->mapper = get_mapper(prog->header.mapper_no);
    if (prog->mapper == NULL) {
        printf("Emulator does not support mapper number %d.\n", prog->header.mapper_no);
        exit(1);
    }

    // Decode remaining flags depending on format.
    ines_flag8_t flag8 = { .bits = (uint8_t)src[8] };
    ines_flag9_t flag9 = { .bits = (uint8_t)src[9] };
    if (prog->header.format == 2) {
        printf("Emulator does not support NES 2.0 file format.\n");
        exit(1);
    }
    else {
        prog->header.prg_ram_size = flag8.flags.prg_ram_size;
        prog->header.tv_sys = flag9.flags.tv_sys;
    }

    // Prepare copying of data.
    const char *fp = src + INES_HEADER_SIZE;
    
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
    prog->prg_rom_sz = prog->header.prg_rom_size * INES_PRG_ROM_UNIT;
    prog->prg_rom = malloc(prog->prg_rom_sz);
    memcpy((void *)prog->prg_rom, fp, prog->prg_rom_sz);
    fp += prog->prg_rom_sz;

    // Copy CHR-ROM.
    if (prog->header.chr_rom_size > 0) {
        prog->chr_sz = prog->header.chr_rom_size * INES_CHR_ROM_UNIT;
        prog->chr_rom = malloc(prog->chr_sz);
        memcpy((void *)prog->chr_rom, fp, prog->chr_sz);
        fp += prog->chr_sz;
    }
    else {
        // Use CHR-RAM instead.
        prog->chr_sz = sizeof(prog->chr_ram);
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

    // Declare PRG-RAM size.
    prog->prg_ram_sz = sizeof(prog->prg_ram);

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
    mapper_destroy(prog->mapper);
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
