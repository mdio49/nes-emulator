#ifndef MAPPERS_H
#define MAPPERS_H

#include <mapper.h>
#include <prog.h>

/* mapper implementation */
struct mapper {

    /**
     * @brief Initialization constructor. The new instance will not have a reference to
     * this function (i.e. it cannot create new instances of itself).
     * 
     * @return The new instance of the mapper.
     */
    mapper_t        *(*init)(void);

    /* function pointers */

    void            (*insert)(mapper_t *mapper, prog_t *prog);
    void            (*write)(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value);

    /* system pointers */

    addrspace_t     *cpuas;     // Reference to CPU address space.
    addrspace_t     *ppuas;     // Reference to PPU address space.
    uint8_t         *vram;      // Reference to PPU VRAM.

    /* mapper registers */

    uint8_t         sr[16];     // Shift registers (16 available).
    uint8_t         *banks;     // Bank registers (mapper can allocate as many as needed).

};

/* mapper singletons */
extern const mapper_t nrom;

/**
 * @brief Invoked when the cartridge is inserted into the system, allowing the mapper
 * to initialize any address space mapping when the system is powered on.
 * 
 * @param mapper The mapper.
 * @param prog The NES program that is using the mapper.
 */
void mapper_insert(mapper_t *mapper, prog_t *prog);

/**
 * @brief Invoked when a CPU write occurs in the catridge's address range, allowing
 * the mapper to update its state and perform any bank switching.
 * 
 * @param mapper The mapper.
 * @param prog The NES program that is using the mapper.
 * @param vaddr The virtual address that was written to.
 * @param value The value that was written.
 */
void mapper_write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value);

#endif
