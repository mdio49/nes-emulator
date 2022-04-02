#include <stdio.h>
#include <stdlib.h>
#include <vm.h>

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

/**
 * @brief A range of virtual memory that maps to an emulator virtual address.
 */
typedef struct mem_seg {
    
    uint32_t        start;          // The start of the address range (inclusive).
    uint32_t        end;            // The end of the address range (exclusive).
    uint8_t         *target;        // The target emulator address that the start of this segment points to.
    struct mem_seg  *next;          // A pointer to the next segment.

    //uint8_t (*map)(addr_t vaaddr);  // The function that maps a virtual address in this address range to an actual value in the emulator's memory.

} mem_seg_t;

struct addrspace {

    mem_seg_t       *first;

};

static mem_seg_t *seg_create(uint32_t start, uint32_t end, uint8_t *target) {
    mem_seg_t *node = malloc(sizeof(struct mem_seg));
    node->start = start;
    node->end = end;
    node->target = target;
    node->next = NULL;
    return node;
}

addrspace_t *as_create() {
    addrspace_t *as = malloc(sizeof(struct addrspace));
    as->first = NULL;
    return as;
}

void as_add_segment(addrspace_t *as, addr_t start, size_t size, uint8_t *target) {
    const uint32_t end = start + size;
    if (as->first == NULL) {
        as->first = seg_create(start, end, target);
    }
    else {
        mem_seg_t *seg = as->first;
        mem_seg_t *prev = NULL;
        while (seg != NULL) {
            if (seg->start >= end) {
                // Segment is after new segment; place new segment.
                mem_seg_t *node = seg_create(start, end, target);
                node->next = seg;
                prev->next = node;
                break;
            }
            else if (seg->end > start) {
                // Segment is within new segment.
                printf("Segment within this memory range already exists.\n");
                exit(1);
            }

            prev = seg;
            seg = seg->next;
        }

        if (seg == NULL) {
            mem_seg_t *node = seg_create(start, end, target);
            prev->next = node;
        }
    }
}

uint8_t *as_traverse(addrspace_t *as, addr_t start, size_t nbytes) {
    uint8_t *result = malloc(sizeof(uint8_t) * nbytes);
    for (mem_seg_t *seg = as->first; seg != NULL; seg = seg->next) {
        for (int i = max(start, seg->start); i < min(start + nbytes, seg->end); i++) {
            result[i - start] = *vaddr_to_ptr(as, i);
        }
    }
    return result;
}

void as_destroy(addrspace_t *as) {
    mem_seg_t *seg = as->first;
    while (seg != NULL) {
        mem_seg_t *next = seg->next;
        free(seg);
        seg = next;
    }
    free(as);
}

uint8_t *vaddr_to_ptr(const addrspace_t *as, addr_t vaddr) {
    uint8_t *result = NULL;
    for (mem_seg_t *seg = as->first; seg != NULL; seg = seg->next) {
        if (vaddr >= seg->end)
            continue;
        if (vaddr < seg->start)
            break;
        result = seg->target + (vaddr - seg->start);
    }

    if (result == NULL) {
        printf("Segmentation fault.\n");
        exit(1);
    }

    return result;
}
