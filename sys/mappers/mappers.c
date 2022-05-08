#include <mappers.h>

#define N_MAPPERS 256

static mapper_t mappers[N_MAPPERS] = {
    { .init = NULL } // NULL the init function by default if a mapper isn't supported.
};

static inline void init_mappers(void) {
    mappers[0] = nrom;
    // ...
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
