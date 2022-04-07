#include "gdevice.h"
#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shader.h>

static GLFWwindow *window = NULL;
static int vao, vbo;
static int shader;

static int pixel_data[256 * 256] = { 0 };

static const float vertices[] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,

    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
};

bool gdevice_create() {
    // Initialize GLFW.
	if (!glfwInit()) {
		printf("Failed to initialize GLFW!\n");
		return false;
	}

	// Create window.
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, false);

	window = glfwCreateWindow(800, 600, "OpenGL Program", NULL, NULL);
	if (window == NULL) {
		printf("Failed to create GLFW window!\n");
		glfwTerminate();
		return false;
	}

    glfwMakeContextCurrent(window);

	// Initialize GLAD.
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Failed to initialize GLAD!\n");
		return false;
	}

	glViewport(0, 0, 256, 240);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Create and bind VAO.
	glGenVertexArrays(1, vao);
	glBindVertexArray(vao);

	// Create and bind VBO.
	glGenBuffers(1, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	// Create shader.
	shader_t *vertex = shader_fromfile(GL_VERTEX_SHADER, "graphics/shaders/main.vert");
	shader_t *frag = shader_create(GL_FRAGMENT_SHADER, "graphics/shaders/main.frag");

	shader_compile(vertex);
	shader_compile(frag);

	shader = glCreateProgram();
	glAttachShader(shader, vertex->id);
	glAttachShader(shader, frag->id);
	glLinkProgram(shader);
	
	free(vertex->src);
	free(frag->src);

	shader_destroy(vertex);
	shader_destroy(frag);

	glUseProgram(shader);

    return true;
}

void gdevice_draw() {
    glClear(GL_COLOR_BUFFER_BIT);

	int data = glGetUniformLocation(shader, "data");
	glUniform1iv(data, 256 * 256, pixel_data);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void gdevice_destroy() {
    if (window != NULL)
        glfwDestroyWindow(window);
    glfwTerminate();
}
