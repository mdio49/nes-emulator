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
 * @brief A range of virtual memory that maps to an emulator virtual address.
 */
typedef struct addrange {
    
    addr_t      start;              // The start of the address range.
    addr_t      end;                // The end of the address range.

    uint8_t (*map)(addr_t vaaddr);  // The function that maps a virtual address in this address range to an actual value in the emulator's memory.

} addrange_t;

/**
 * @brief An area of addressable memory that maps a virtual memory address to a "physical" memory location (i.e. am emulator virtual memory address).
 */
typedef struct addrspace {

#ifdef DUMB_VM
    uint8_t     *mem;               // Simplest case, we have one pointer of contiguous memory.
#else
    // TODO
#endif

} addrspace_t;

/**
 * @brief Maps a virtual address from the given address space into a "physical" memory location.
 * 
 * @param as The address space.
 * @param vaddr The virtual address.
 * @return A pointer that corresponds to the mapped location of the virtual address.
 */
uint8_t *vaddr_to_ptr(const addrspace_t *as, addr_t vaddr);

#endif
