#include <mappers.h>
#include <stdlib.h>

#define N_MAPPERS 256

static mapper_t mappers[N_MAPPERS] = {
    { .init = NULL } // NULL the init function by default if a mapper isn't supported.
};

static inline void init_mappers(void) {
    mappers[0] = nrom;
    mappers[1] = mmc1;
    mappers[2] = uxrom;
    mappers[3] = ines003;
    mappers[4] = mmc3;
    // ...
}

static uint8_t *default_map(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t *target, size_t offset) {
    return target; // Default mapping behaviour where banks are fixed.
}

mapper_t *get_mapper(int number) {
    // Lazy loading of mappers.
    static bool mapper_init = false;
    if (!mapper_init) {
        init_mappers();
        mapper_init = true;
    }

    // Return NULL if the number is out of range of the supported mappers.
    if (number < 0 || number >= N_MAPPERS)
        return NULL;

    // Get the mapper corresponding to the number.
    const mapper_t mapper = mappers[number];

    // Initialize a new instance of the mapper if it is supported; otherwise return NULL.
    return mapper.init != NULL ? mapper.init() : NULL;
}

mapper_t *mapper_create(void) {
    // Allocate memory for the mapper.
    mapper_t *mapper = malloc(sizeof(struct mapper));
    
    // Mapper functions default to fixed banks if the mapper doesn't assign them to anything.
    mapper->map_ram = default_map;
    mapper->map_prg = default_map;
    mapper->map_chr = default_map;
    mapper->map_nts = default_map;
    
    // By default, the mapper contains no bank registers and uses no additional data.
    mapper->banks = NULL;
    mapper->data = NULL;

    // Ensure that IRQ flag is clear.
    mapper->irq = false;
    
    return mapper;
}

void mapper_init(mapper_t *mapper, addrspace_t *cpuas, addrspace_t *ppuas, uint8_t *vram) {
    mapper->cpuas = cpuas;
    mapper->ppuas = ppuas;
    mapper->vram = vram;
}

void mapper_destroy(mapper_t *mapper) {
    if (mapper->banks != NULL)
        free(mapper->banks);
    if (mapper->data != NULL)
        free(mapper->data);
    free(mapper);
}

void mapper_insert(mapper_t *mapper, prog_t *prog) {
    mapper->insert(mapper, prog);
}

void mapper_write(mapper_t *mapper, prog_t *prog, addr_t vaddr, uint8_t value) {
    mapper->write(mapper, prog, vaddr, value);
}
