#include <stdbool.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

typedef struct shader {
    
    const int       id;
    const GLuint64  type;
    const char      *src;
    bool            compiled;

} shader_t;

shader_t *shader_create(unsigned char type, const char *src);
void shader_compile(shader_t *shader);
void shader_destroy(shader_t *shader);
