#include <mappers.h>
#include <stdlib.h>

static mapper_t *init(void);

const mapper_t nrom = {
    .init = init
};

mapper_t *init(void) {
    mapper_t *mapper = malloc(sizeof(struct mapper));
    mapper->banks = NULL;

    return mapper;
}
