#include "shader.h"
#include <stdio.h>
#include <stdlib.h>

shader_t *shader_create(GLuint64 type, const char *src) {
    shader_t shader = {
        .id = glCreateShader(type),
        .type = type,
        .src = src,
        .compiled = false
    };

    shader_t *ret = malloc(sizeof(struct shader));
    memcpy(ret, &shader, sizeof(struct shader));
    return ret;
}

void shader_compile(shader_t *shader) {
    if (shader->compiled) {
        fprintf(stderr, "Shader already compiled.");
        exit(1);
    }

    // Specify the shader source and compile the shader.
	glShaderSource(shader->id, 1, &shader->src, NULL);
	glCompileShader(shader->id);

	// Check if the compilation was successful; if it wasn't, then throw an error.
	int success;
	glGetShaderiv(shader->id, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader->id, 512, NULL, infoLog);
		fprintf(stderr, infoLog);
        exit(1);
	}
	
	shader->compiled = true;
}

void shader_destroy(shader_t *shader) {
    glDeleteShader(shader->id);
    free(shader);
}
