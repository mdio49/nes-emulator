#include <color.h>

static color_t colors[MAX_COLOR + 1] = DEFAULT_COLORS;

color_t color_resolve(uint8_t index) {
    if (index > MAX_COLOR) {
        const color_t black = { 0, 0, 0 };
        return black;
    }
    return colors[index];
}
