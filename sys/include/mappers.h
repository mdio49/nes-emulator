#ifndef MAPPERS_H
#define MAPPERS_H

#include <mapper.h>
#include <prog.h>

#define N_PRG_BANKS(prog, sz) (prog->header.prg_rom_size * INES_PRG_ROM_UNIT / sz)
#define N_CHR_BANKS(prog, sz) (prog->header.chr_rom_size * INES_CHR_ROM_UNIT / sz)

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

    /* function pointers (must be declared by mapper)  */

    void            (*insert)(mapper_t *mapper, prog_t *prog);
    void            (*monitor)(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write);

    /* mapper functions (do not need to be declared by mapper) */

    map_rule_t      map_ram;    // Maps PRG-RAM.
    map_rule_t      map_prg;    // Maps PRG-ROM.
    map_rule_t      map_chr;    // Maps CHR-ROM/RAM.
    map_rule_t      map_nts;    // Maps nametables.

    /* additional functions (do not need to be declared by mapper) */

    void            (*cycle)(mapper_t *mapper, prog_t *prog, int cycles);
    float           (*mix)(mapper_t *mapper, prog_t *prog, float input);

    /* system pointers */

    addrspace_t     *cpuas;     // Reference to CPU address space.
    addrspace_t     *ppuas;     // Reference to PPU address space.
    uint8_t         *vram;      // Reference to PPU VRAM.

    /* mapper registers */

    uint8_t         r8[16];     // 8-bit registers (16 available).
    uint16_t        r16[8];     // 16-bit registers (8 available).
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
 * @brief Invoked whenever a read or write occurs to either the CPU or PPU's address
 * space, allowing the mapper to monitor reads and writes on the bus so that it may
 * update its state and perform any bank switching. As the mapper doesn't have a clock,
 * this is the only way that the mapper can keep track of its state.
 * 
 * @param mapper The mapper.
 * @param prog The NES program that is using the mapper.
 * @param as The address space that was modified.
 * @param vaddr The virtual address that was accessed.
 * @param value The value that was read or written.
 * @param write Set if the access was a write.
 */
void mapper_monitor(mapper_t *mapper, prog_t *prog, addrspace_t *as, addr_t vaddr, uint8_t value, bool write);

/**
 * @brief Invoked whenever a CPU cycle occurs (or when a series of cycles occur). The
 * mapper can detect this via a rising edge of the M2 pin; for emulation purposes, this
 * method is simply called by the system at the end of a CPU cycle.
 * 
 * @param mapper The mapper.
 * @param prog The NES program that is using the mapper.
 * @param cycles The number of cycles that have elapsed (should be 1 if CPU is implemented faithfully).
 */
void mapper_cycle(mapper_t *mapper, prog_t *prog, int cycles);

/**
 * @brief Invoked whenever an APU cycle occurs, allowing the mapper to manipulate the
 * mixer output in order to add additional audio data.
 * 
 * @param mapper The mapper.
 * @param prog The NES program that is using the mapper.
 * @param input The current value ready to be sent to the mixer.
 * @return The new value that will be outputted to the mixer.
 */
float mapper_mix(mapper_t *mapper, prog_t *prog, float input);

/* mapper singletons */
extern const mapper_t nrom, mmc1, uxrom, ines003, mmc3, mmc5, mmc2;

#endif
