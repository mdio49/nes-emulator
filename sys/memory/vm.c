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
    
    /* segment range */
    uint32_t        start;          // The start of the address range (inclusive).
    uint32_t        end;            // The end of the address range (exclusive).

    /* segment target */
    uint8_t         *target;        // The target emulator address that the start of this segment points to.
    size_t          offset;         // The offset added to the target when resolving.

    /* linked list */
    struct mem_seg  *next;          // A pointer to the next segment.

} mem_seg_t;

typedef struct as_mirror {
    
    addr_t          start;          // The start of the mirror (inclusive).
    addr_t          end;            // The end of the the mirror (inclusive).
    addr_t          target;         // The target virtual address.
    size_t          repeat;         // The amount to repeat by.

    struct as_mirror   *next;      // A pointer to the next mirror.

} as_mirror_t;

struct addrspace {

    mem_seg_t       *segs[N_SEGS];  // A pointer to the head of the list of segments for each 2KB block.

    as_mirror_t     *mirrors;       // A pointer to the head of the list of mirrors in the address space.
    as_mirror_t     *mirrors_tail;  // A pointer to the head of the list of mirrors in the address space.

    resolve_rule_t  resolve_rule;   // The resolve rule that is called whenver a virtual address is resolved.
    update_rule_t   update_rule;    // The update rule that is called whenever a virtual address is accessed.

};

static inline uint8_t *resolve_vaddr(const addrspace_t *as, addr_t vaddr) {
    uint8_t *target = NULL;
    size_t offset = 0;

    // Evaluate mirrors.
    for (as_mirror_t *mirror = as->mirrors; mirror != NULL; mirror = mirror->next) {
        if (vaddr < mirror->start || vaddr > mirror->end)
            continue;

        size_t offset = vaddr - mirror->start;
        if (mirror->repeat > 0) {
            offset %= mirror->repeat;
        }
        vaddr = mirror->target + offset;
    }

    // Evaluate resultant virtual address.
    const mem_seg_t *head = as->segs[vaddr / SEG_SIZE];
    for (const mem_seg_t *seg = head; seg != NULL; seg = seg->next) {
        if (vaddr >= seg->end)
            continue;
        if (vaddr < seg->start)
            break;

        offset = seg->offset + (vaddr - seg->start);
        target = seg->target + offset;
    }

    // Apply the resolve rule (if it exists).
    if (as->resolve_rule != NULL) {
        target = as->resolve_rule(as, vaddr, target, offset);
    }

    // Exit with a "segmentation fault" if the address couldn't be resolved.
    if (target == NULL) {
        printf("Segmentation fault ($%.4x).\n", vaddr);
        as_print(as);
        exit(1);
    }

    return target;
}

addrspace_t *as_create() {
    addrspace_t *as = malloc(sizeof(struct addrspace));
    for (int i = 0; i < N_SEGS; i++) {
        as->segs[i] = NULL;
    }
    as->mirrors = NULL;
    as->mirrors_tail = NULL;
    as->resolve_rule = NULL;
    as->update_rule = NULL;
    return as;
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

void as_add_segment(addrspace_t *as, addr_t start, size_t size, uint8_t *target) {
    const uint32_t end = start + size;
    for (int i = start / SEG_SIZE; i <= (end - 1) / SEG_SIZE; i++) {
        // Determine the start and end of the new segment.
        const uint32_t new_start = max(start, i * SEG_SIZE);
        const uint32_t new_end = min(end, (i + 1) * SEG_SIZE);

        // Create the new segment.
        mem_seg_t *new_seg =  malloc(sizeof(struct mem_seg));
        new_seg->start = new_start;
        new_seg->end = new_end;
        new_seg->offset = new_start - start;
        new_seg->target = target;
        new_seg->next = NULL;

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
                    seg = NULL;
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
                    seg->offset += new_seg->end - seg->start;
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
            
            if (seg != NULL) {
                prev = seg;
            }
            seg = next;
        }

        // If we've reached the end of the list, then the segment goes after the last segment.
        if (seg == NULL) {
            prev->next = new_seg;
        }
    }
}

void as_add_mirror(addrspace_t *as, addr_t start, addr_t end, size_t repeat, addr_t target) {
    as_mirror_t *mirror = malloc(sizeof(struct as_mirror));
    mirror->start = start;
    mirror->end = end;
    mirror->repeat = repeat;
    mirror->target = target;
    mirror->next = NULL;
    if (as->mirrors == NULL) {
        as->mirrors = mirror;
    }
    else {
        as->mirrors_tail->next = mirror;
    }
    as->mirrors_tail = mirror;
}

void as_set_resolve_rule(addrspace_t *as, resolve_rule_t rule) {
    as->resolve_rule = rule;
}

void as_set_update_rule(addrspace_t *as, update_rule_t rule) {
    as->update_rule = rule;
}

uint8_t as_read(const addrspace_t *as, addr_t vaddr) {
    //printf("R %p: $%.4x\n", as, vaddr);
    uint8_t value = *resolve_vaddr(as, vaddr);
    if (as->update_rule != NULL) {
        value = as->update_rule(as, vaddr, value, AS_READ);
    }
    return value;
}

void as_write(const addrspace_t *as, addr_t vaddr, uint8_t value) {
    //printf("W: %p $%.4x\n", as, vaddr);
    if (as->update_rule != NULL) {
        value = as->update_rule(as, vaddr, value, AS_WRITE);
    }
    *resolve_vaddr(as, vaddr) = value;
}

uint8_t *as_traverse(const addrspace_t *as, addr_t start, size_t nbytes) {
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

void as_print(const addrspace_t *as) {
    for (int i = 0; i < N_SEGS; i++) {
        const mem_seg_t *head = as->segs[i];
        for (const mem_seg_t *seg = head; seg != NULL; seg = seg->next) {
            printf("$%.4x - $%.4x -> 0x%p\n", seg->start, seg->end, seg->target);
        }
    }
}
