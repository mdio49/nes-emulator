#include <color.h>

static color_t colors[MAX_COLOR + 1] = DEFAULT_COLORS;

color_t color_resolve(uint8_t index) {
    if (index > MAX_COLOR) {
        return colors[0x0F];
    }
    return colors[index];
}
