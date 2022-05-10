#ifndef VM_H
#define VM_H

#include <stdint.h>

#define AS_READ     0x01
#define AS_WRITE    0x02

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
 * @brief A resolve rule (occurs whenever a virtual address is resolved).
 * 
 * @param as The address space.
 * @param vaddr The virtual address that is being resolved.
 * @param target The current target address (can be remapped via the return value).
 * @param offset The offset of the virtual address from the start of the segment.
 * @return The resolved physical/emulator address.
 */
typedef uint8_t *(*resolve_rule_t)(const addrspace_t *as, addr_t vaddr, uint8_t *target, size_t offset);

/**
 * @brief An update rule (occurs whenever an address is read from or written to).
 * 
 * @param as The address space that was updated.
 * @param vaddr The virtual address that was updated.
 * @param value The value at the address before the update.
 * @param mode Whether the update was a read or a write.
 * @return The value that gets read, or the value to write to the virtual address.
 */
typedef uint8_t (*update_rule_t)(const addrspace_t *as, addr_t vaddr, uint8_t value, uint8_t mode);

/**
 * @brief Creates an empty address space.
 * 
 * @return The resultant address space.
 */
addrspace_t *as_create();

/**
 * @brief Destroys the given address space. Does not free any underlying memory that this address space references.
 * 
 * @param as The address space to destroy.
 */
void as_destroy(addrspace_t *as);

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
 * @brief Adds a mirror to the address space that causes a segment of memory to link to another
 * segment of memory within the address space. When resolving virtual addresses, the address
 * is first passed through all mirrors before resolving the address to a segment. Mirrors are
 * evaluated once in the order that they are defined.
 * 
 * @param as The address space to modify.
 * @param start The start of the mirror.
 * @param end The end of the mirror (inclusive).
 * @param repeat The interval to repeat within the mirror (set to 0 to use entire region).
 * @param target The target virtual address within the address space.
 */
void as_add_mirror(addrspace_t *as, addr_t start, addr_t end, size_t repeat, addr_t target);

/**
 * @brief Sets the resolve rule that is called whenever a virtual address in the given address
 * space is resolved into a "physical" address in the emulator's virtual address space.
 * 
 * @param as The address space.
 * @param rule The resolve rule (or `NULL` to clear the rule).
 */
void as_set_resolve_rule(addrspace_t *as, resolve_rule_t rule);

/**
 * @brief Sets the update rule that is called whenever a memory address in the given address
 * space is accessed (i.e. read from or written to).
 * 
 * @param as The address space.
 * @param rule The update rule (or `NULL` to clear the rule).
 */
void as_set_update_rule(addrspace_t *as, update_rule_t rule);

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
uint8_t *as_traverse(const addrspace_t *as, addr_t start, size_t nbytes);

/**
 * @brief Prints the address space, showing virtual memory ranges and the physical memory location
 * that the segment references.
 * 
 * @param as The address space to print.
 */
void as_print(const addrspace_t *as);

#endif
