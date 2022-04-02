#ifndef VM_H
#define VM_H

#define DUMB_VM 1

#include <stdint.h>

/**
 * @brief Memory addresses are 16-bit ranging from 0x00 to 0xFF.
 */
typedef uint16_t addr_t;

/**
 * @brief A struct containing a virtual address along with a pointer to the value that it points to.
 */
typedef struct vaddr_ptr_pair {

    /**
     * @brief The virtual address. May not be valid if the value points to somewhere not in the
     * address space that this virtual address is associated with.
     */
    addr_t      vaddr;

    /**
     * @brief A pointer to the value corresponding to the virtual address.
     */
    uint8_t     *ptr;

} vaddr_ptr_pair_t;


/**
 * @brief An area of addressable memory that maps a virtual memory address to a "physical" memory location (i.e. am emulator virtual memory address).
 */
typedef struct addrspace addrspace_t;

/**
 * @brief Creates an empty address space.
 * 
 * @return The resultant address space.
 */
addrspace_t *as_create();

/**
 * @brief Adds a segment of memory to the given address space.
 * 
 * @param as The address space to modify.
 * @param start The start address.
 * @param size The size of the segment.
 * @param target The target memory that the segment points to.
 */
void as_add_segment(addrspace_t *as, addr_t start, size_t size, uint8_t *target);

/**
 * @brief Traverses the address space, placing any memory found into a new array.
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

/**
 * @brief Maps a virtual address from the given address space into a "physical" memory location.
 * 
 * @param as The address space.
 * @param vaddr The virtual address.
 * @return A pointer that corresponds to the mapped location of the virtual address.
 */
uint8_t *vaddr_to_ptr(const addrspace_t *as, addr_t vaddr);

#endif
