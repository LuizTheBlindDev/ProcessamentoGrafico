//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "gl_utils.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

int g_gl_width = 480;
int g_gl_height = 480;
GLFWwindow* g_window = NULL;

int main()
{
	restart_gl_log();
	start_gl();
	float posX = 0.0f, posY = 0.0f;
	float speed = 0.01f;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	float vertices[] = {
		0.5f,  0.5f, 1.0f, 1.0f,
		0.5f, -0.5f, 1.0f, 0.0f,
		-0.5f, -0.5f, 0.0f, 0.0f,
		-0.5f,  0.5f, 0.0f, 1.0f
	};
	unsigned int indices[] = { 2, 1, 0, 0, 3, 2 };

	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	char vertex_shader[1024 * 256];
	char fragment_shader[1024 * 256];
	parse_file_into_str("_sprites_vs.glsl", vertex_shader, 1024 * 256);
	parse_file_into_str("_sprites_fs.glsl", fragment_shader, 1024 * 256);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* p = (const GLchar*)vertex_shader;
	glShaderSource(vs, 1, &p, NULL);
	glCompileShader(vs);

	int params = -1;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) { print_shader_info_log(vs); return 1; }

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	p = (const GLchar*)fragment_shader;
	glShaderSource(fs, 1, &p, NULL);
	glCompileShader(fs);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) { print_shader_info_log(fs); return 1; }

	GLuint shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);
	glGetProgramiv(shader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) { return false; }

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

	int width, height, nrChannels;
	unsigned char* data = stbi_load("sully.png", &width, &height, &nrChannels, STBI_rgb_alpha);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	stbi_image_free(data);

	float fw = 0.25f, fh = 0.25f;
	float offsetx = 0, offsety = 0;
	int frameAtual = 0;
	int acao = 3;
	float previous = glfwGetTime();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	while (!glfwWindowShouldClose(g_window)) {
		_update_fps_counter(g_window);
		double current_seconds = glfwGetTime();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, g_gl_width, g_gl_height);
		glUseProgram(shader_programme);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);

		bool moving = false;
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_W) || GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_UP)) {
			acao = 3; 
			posY += speed;
			moving = true;
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_D) || GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_RIGHT)) {
			acao = 2; 
			posX += speed;
			moving = true;
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_A) || GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_LEFT)) {
			acao = 1; 
			posX -= speed;
			moving = true;
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_S) || GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_DOWN)) {
			acao = 0; 
			posY -= speed;
			moving = true;
		}

		if ((current_seconds - previous) > 0.16) {
			previous = current_seconds;
			frameAtual = moving ? (frameAtual + 1) % 4 : 0;
			offsetx = fw * (float)frameAtual;
			offsety = fh * (3 - acao);  // invertido
		}

		glUniform1f(glGetUniformLocation(shader_programme, "offsetx"), offsetx);
		glUniform1f(glGetUniformLocation(shader_programme, "offsety"), offsety);
		glUniform2f(glGetUniformLocation(shader_programme, "spritePos"), posX, posY);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glfwPollEvents();
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(g_window, 1);
		}
		glfwSwapBuffers(g_window);
	}
	glfwTerminate();
	return 0;
}
