#ifndef VM_H
#define VM_H

#include <stdint.h>

/**
 * @brief Memory addresses are 16-bit ranging from 0x0000 to 0xFFFF.
 */
typedef uint16_t addr_t;

/**
 * @brief An area of addressable memory that maps a virtual memory address to a "physical" memory
 * location (i.e. an emulator virtual memory address).
 */
typedef struct addrspace addrspace_t;

/**
 * @brief Creates an empty address space.
 * 
 * @return The resultant address space.
 */
addrspace_t *as_create();

/**
 * @brief Adds a segment of memory to the given address space. If the segment overlaps with any
 * existing segments, then those segments will be resized in order to fit the new segment.
 * 
 * @param as The address space to modify.
 * @param start The start address.
 * @param size The size of the segment.
 * @param target The target memory that the segment points to.
 */
void as_add_segment(addrspace_t *as, addr_t start, size_t size, uint8_t *target);

/**
 * @brief Reads the value at the memory location corresponding to the given virtual address. If
 * the address space does not have a segment that contains the given virtual address, then this
 * method will produce a segmentation error.
 * 
 * @param as The address space.
 * @param vaddr The virtual address.
 * @return The value at the virtual address.
 */
uint8_t as_read(const addrspace_t *as, addr_t vaddr);

/**
 * @brief Writes a value to the memory location corresponding to the given virtual address. If
 * the address space does not have a segment that contains the given virtual address, then this
 * method will produce a segmentation error.
 * 
 * @param as The address space.
 * @param vaddr The virtual address.
 * @param value The value to write to the virtual address.
 */
void as_write(const addrspace_t *as, addr_t vaddr, uint8_t value);

/**
 * @brief Traverses the address space, copying any memory found into a new array. Any memory that is
 * not present in the address space will not be filled into the array.
 * 
 * @param as The address space to traverse.
 * @param start The virtual address to start at.
 * @param nbytes The number of bytes to traverse.
 * @return An array corresponding to each byte of the traversal.
 */
uint8_t *as_traverse(addrspace_t *as, addr_t start, size_t nbytes);

/**
 * @brief Destroys the given address space. Does not free any underlying memory that this address space references.
 * 
 * @param as The address space to destroy.
 */
void as_destroy(addrspace_t *as);

#endif