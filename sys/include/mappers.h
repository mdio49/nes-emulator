#ifndef MAPPERS_H
#define MAPPERS_H

#include <mapper.h>
#include <prog.h>

#define PRG_RAM_START   0x6000
#define PRG_ROM_START   0x8000

/**
 * @brief Mapper rule (takes a similar form to an address space resolve rule).
 * 
 * @param mapper The mapper.
 * @param prog The NES program that is using the mapper.
 * @param vaddr The virtual address that is being mapped.
 * @param target The mapped address prior to performing any mapping.
 * @param offset The offset of the virtual address from the start of the segment
 *               that the address is contained in within the address space.
 * @return The resultant physical/emulator address that gets mapped.
 */
typedef uint8_t *(*map_rule_t)(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset);

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

    /* mapper functions */

    map_rule_t      map_ram;    // Maps PRG-RAM.
    map_rule_t      map_prg;    // Maps PRG-ROM.
    map_rule_t      map_chr;    // Maps CHR-ROM/RAM.
    map_rule_t      map_nts;    // Maps nametables.

    /* system pointers */

    addrspace_t     *cpuas;     // Reference to CPU address space.
    addrspace_t     *ppuas;     // Reference to PPU address space.
    uint8_t         *vram;      // Reference to PPU VRAM.

    /* mapper registers */

    uint8_t         sr[16];     // Shift registers (16 available).
    uint8_t         *banks;     // Bank registers (mapper can allocate as many as needed).

    /* other variables */

    bool            irq;        // Set if the CPU should generate an IRQ on its next instruction fetch.    
    void            *data;      // Additional data that the mapper may allocate.

};

/**
 * @brief Creates a new instance of an empty mapper with all necessary variables
 * initialized to their default values.
 * 
 * @return The new mapper instance.
 */
mapper_t *mapper_create(void);

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

/* mapper singletons */
extern const mapper_t nrom, mmc1, uxrom, ines003, mmc3;

#endif
