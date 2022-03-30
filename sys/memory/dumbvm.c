#include <vm.h>

uint8_t *vaddr_to_ptr(const addrspace_t *as, addr_t vaddr) {
    return as->mem + vaddr;
}
