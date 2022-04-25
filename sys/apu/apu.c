#include <apu.h>
#include <stdlib.h>

apu_t *apu_create(void) {
    apu_t *apu = malloc(sizeof(struct apu));
    return apu;
}

void apu_destroy(apu_t *apu) {
    free(apu);
}
