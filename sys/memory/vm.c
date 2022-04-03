#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vm.h>

#define N_SEGS 32
#define SEG_SIZE (65536 / N_SEGS)

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

    mem_seg_t       *segs[N_SEGS];  // A pointer to the head of the list of segments for each 2KB block.

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
    for (int i = 0; i < N_SEGS; i++) {
        as->segs[i] = NULL;
    }
    return as;
}

void as_add_segment(addrspace_t *as, addr_t start, size_t size, uint8_t *target) {
    const uint32_t end = start + size;
    for (int i = start / SEG_SIZE; i <= (end - 1) / SEG_SIZE; i++) {
        // Create the new segment.
        const uint32_t new_start = max(start, i * SEG_SIZE);
        const uint32_t new_end = min(end, (i + 1) * SEG_SIZE);
        mem_seg_t *new_seg = seg_create(new_start, new_end, target + (new_start - start));

        // If this is the first segment in the region, then add it and continue.
        if (as->segs[i] == NULL) {
            as->segs[i] = new_seg;
            continue;
        }

        // Otherwise, update the list of segments in the region to fit the new segment.
        mem_seg_t *seg = as->segs[i];
        mem_seg_t *prev = NULL;
        while (seg != NULL) {
            mem_seg_t *next = seg->next;
            if (seg->start >= new_seg->end) {
                // Segment is after new segment; place new segment.
                new_seg->next = seg;
                if (prev != NULL) {
                    prev->next = new_seg;
                }
                else {
                    as->segs[i] = new_seg;
                }
                break;
            }
            else if (seg->end > new_seg->start) {
                // Segment is within new segment.
                if (seg->start >= new_seg->start && seg->end <= new_seg->end) {
                    // Segment is fully contained within new segment (delete old segment).
                    new_seg->next = seg->next;
                    if (prev != NULL) {
                        prev->next = new_seg;
                    }
                    else {
                        as->segs[i] = new_seg;
                    }
                    free(seg);
                }
                else if (seg->start >= new_seg->start && seg->start < new_seg->end) {
                    // Segment intersects with new segment at end.
                    new_seg->next = seg;
                    if (prev != NULL) {
                        prev->next = new_seg;
                    }
                    else {
                        as->segs[i] = new_seg;
                    }
                    seg->start = new_seg->end;
                    break;
                }
                else if (seg->start < new_seg->start && seg->end > new_seg->start) {
                    // Segment intersects with new segment at start.
                    new_seg->next = seg->next;
                    seg->next = new_seg;
                    seg->end = new_seg->start;
                }
            }

            prev = seg;
            seg = next;
        }

        // If we've reached the end of the list, then the segment goes after the last segment.
        if (seg == NULL) {
            prev->next = new_seg;
        }
    }
}

uint8_t *as_resolve(const addrspace_t *as, addr_t vaddr) {
    uint8_t *result = NULL;
    const mem_seg_t *head = as->segs[vaddr / SEG_SIZE];
    for (const mem_seg_t *seg = head; seg != NULL; seg = seg->next) {
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

uint8_t *as_traverse(addrspace_t *as, addr_t start, size_t nbytes) {
    uint8_t *result = calloc(nbytes, sizeof(uint8_t));
    const uint32_t end = start + nbytes;
    for (int i = start / SEG_SIZE; i <= (end - 1) / SEG_SIZE; i++) {
        for (mem_seg_t *seg = as->segs[i]; seg != NULL; seg = seg->next) {
            const uint32_t cpy_start = max(start, seg->start);
            const size_t cpy_bytes = min(end, seg->end) - cpy_start;
            memcpy(result + (cpy_start - start), seg->target + (cpy_start - seg->start), cpy_bytes);
        }
    }
    return result;
}

void as_destroy(addrspace_t *as) {
    for (int i = 0; i < N_SEGS; i++) {
        mem_seg_t *seg = as->segs[i];
        while (seg != NULL) {
            mem_seg_t *next = seg->next;
            free(seg);
            seg = next;
        }
    }
    
    free(as);
}
