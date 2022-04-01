/**
 * @file dumbvm.c
 * @brief A dumb virtual memory implementation that uses a static 64KB array to address memory.
 * @version 1.0
 * @date 2022-03-31
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>
#include <vm.h>

addrspace_t *as_create() {
    addrspace_t *as = malloc(sizeof(struct addrspace));
    as->mem = NULL;
    return as;
}

void as_destroy(addrspace_t *as) {
    free(as);
}

uint8_t *vaddr_to_ptr(const addrspace_t *as, addr_t vaddr) {
    return as->mem + vaddr;
}
