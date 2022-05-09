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
typedef struct mapper mapper_t;

/**
 * @brief Gets the mapper that corresponds to the given mapper number, or NULL if the
 * mapper is not supported by the emulator.
 * 
 * @param number The mapper number.
 * @return A new instance of the mapper (or NULL if not supported).
 */
mapper_t *get_mapper(int number);

/**
 * @brief Initializes the mapper. The mapper is unusable before this function is called.
 * 
 * @param mapper The mapper to initialize.
 * @param cpuas The CPU address space.
 * @param ppuas The PPU address space.
 * @param vram The PPU's VRAM.
 */
void mapper_init(mapper_t *mapper, addrspace_t *cpuas, addrspace_t *ppuas, uint8_t *vram);

/**
 * @brief Destroys the mapper, freeing any associated resources.
 * 
 * @param mapper The mapper to free.
 */
void mapper_destroy(mapper_t *mapper);

#endif 
