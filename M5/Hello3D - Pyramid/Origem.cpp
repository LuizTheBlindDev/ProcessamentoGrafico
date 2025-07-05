#include "stb_image.h"
#include "gl_utils.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

int g_gl_width = 480;
int g_gl_height = 480;
GLFWwindow* g_window = NULL;

class Sprite {
public:
    GLuint VAO, VBO, EBO;
    GLuint textureID;
    GLuint shader;
    float fw = 0.25f, fh = 0.25f;
    int frameAtual = 0;
    int acao = 3;
    float offsetx = 0.0f, offsety = 0.0f;
    glm::vec2 position;
    float lastUpdate = 0.0f;

    Sprite(GLuint shader, const std::string& path)
        : shader(shader), position(0.0f, 0.0f) {
        initGeometry();
        loadTexture(path);
    }

    void update(float currentTime, bool moving) {
        if ((currentTime - lastUpdate) > 0.16f) {
            lastUpdate = currentTime;
            frameAtual = moving ? (frameAtual + 1) % 4 : 0;
            offsetx = fw * frameAtual;
            offsety = fh * (3 - acao);
        }
    }

    void draw() {
        glUseProgram(shader);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shader, "sprite"), 0);
        glUniform1f(glGetUniformLocation(shader, "offsetx"), offsetx);
        glUniform1f(glGetUniformLocation(shader, "offsety"), offsety);
        glUniform2f(glGetUniformLocation(shader, "spritePos"), position.x, position.y);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    void setDirection(int dir) { acao = dir; }
    void move(float dx, float dy) { position += glm::vec2(dx, dy); }

private:
    void initGeometry() {
        float vertices[] = {
            0.5f,  0.5f, 1.0f, 1.0f,
            0.5f, -0.5f, 1.0f, 0.0f,
           -0.5f, -0.5f, 0.0f, 0.0f,
           -0.5f,  0.5f, 0.0f, 1.0f
        };
        unsigned int indices[] = { 2, 1, 0, 0, 3, 2 };

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
    }

    void loadTexture(const std::string& path) {
        int width, height, nrChannels;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        GLfloat max_aniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        stbi_image_free(data);
    }
};

GLuint compileShader() {
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
    if (GL_TRUE != params) { return 1; }

    return shader_programme;
}

int main() {
    restart_gl_log();
    start_gl();
    float speed = 0.01f;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    GLuint shader_programme = compileShader();
    Sprite sully(shader_programme, "sully.png");
    float previous = glfwGetTime();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    while (!glfwWindowShouldClose(g_window)) {
        _update_fps_counter(g_window);
        double current_seconds = glfwGetTime();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, g_gl_width, g_gl_height);

        bool moving = false;

        if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_W) || GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_UP)) {
            sully.setDirection(3);
            sully.move(0.0f, speed);
            moving = true;
        }
        if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_D) || GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_RIGHT)) {
            sully.setDirection(2);
            sully.move(speed, 0.0f);
            moving = true;
        }
        if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_A) || GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_LEFT)) {
            sully.setDirection(1);
            sully.move(-speed, 0.0f);
            moving = true;
        }
        if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_S) || GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_DOWN)) {
            sully.setDirection(0);
            sully.move(0.0f, -speed);
            moving = true;
        }

        sully.update(current_seconds, moving);
        sully.draw();

        glfwPollEvents();
        if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(g_window, 1);
        }
        glfwSwapBuffers(g_window);
    }

    glfwTerminate();
    return 0;
}
