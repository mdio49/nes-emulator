#ifndef PROG_H
#define PROG_H

#include <ines.h>
#include <mapper.h>

/**
 * @brief A struct that holds data about an NES program.
 */
typedef struct prog {

    ines_header_t   header;             // header
    mapper_t        *mapper;            // mapper
    
    const char      *trainer;           // trainer (may not be present; 0 or 512 bytes)
    const char      *prg_rom;           // PRG-ROM data
    const char      *chr_rom;           // CHR-ROM data
    const char      *inst_rom;          // play-choice INST-ROM data (may not be present; 0 or 8192 bytes)
    const char      *prom;              // play-choice PROM data (may not be present; 0 or 16 bytes)

    uint8_t         prg_ram[0x2000];    // PRG-RAM (may not be used)
    uint8_t         chr_ram[0x2000];    // CHR-RAM (may not be used)

} prog_t;

/**
 * @brief Creates an NES program from the given ROM source.
 * 
 * @param src The source data.
 * @return The program, or NULL if there was not enough memory to create the program.
 */
prog_t *prog_create(const char *src);

/**
 * @brief Destroys the given NES program, freeing any underlying memory associated with it.
 * 
 * @param prog The program to destroy.
 */
void prog_destroy(prog_t *prog);

#endif
