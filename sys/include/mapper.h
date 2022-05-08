#ifndef MAPPER_H
#define MAPPER_H

#include <stdbool.h>
#include <stdint.h>
#include <vm.h>

/**
 * @brief A mapper provides the cartridge with the ability to extend its available
 * ROM data further than the CPU's standard 16-bit (64KB) address space. It accomplishes
 * this primarily via bank switching, which changes the memory that the CPU points to
 * at a particular instance of time during the program's execution.
 */
typedef struct mapper {

    /**
     * @brief Initialization constructor. The new instance will not have a reference to
     * this function (i.e. it cannot create new instances of itself).
     * 
     * @return The new instance of the mapper.
     */
    struct mapper   *(*init)(void);

    uint8_t         sr[16];     // Shift registers (16 available).
    uint8_t         *banks;     // Bank registers (mapper can allocate as many as needed).

} mapper_t;

/**
 * @brief Gets the mapper that corresponds to the given mapper number, or NULL if the
 * mapper is not supported by the emulator.
 * 
 * @param number The mapper number.
 * @return A new instance of the mapper (or NULL if not supported).
 */
mapper_t *get_mapper(int number);

#endif 
