#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

glm::vec2 cameraOffset = glm::vec2(0.0f);
const GLint WIDTH = 800, HEIGHT = 600;
glm::mat4 matrix = glm::mat4(1);

bool pressW = false, pressA = false, pressS = false, pressD = false;

class Sprite {
public:
    GLuint VAO, VBO;
    GLuint textureID;
    GLuint shaderProgram;
    int textureWidth = 0;

    glm::vec2 position;
    glm::vec2 scale;
    float rotation;

    Sprite(GLuint shaderProgram, const std::string& texturePath)
        : shaderProgram(shaderProgram), position(0.0f), scale(1.0f), rotation(0.0f)
    {
        initGeometry();
        loadTexture(texturePath);
    }

    void draw(const glm::mat4& proj) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, 1.0f));

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "matrix"), 1, GL_FALSE, glm::value_ptr(model));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shaderProgram, "basic_texture"), 0);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void drawTiled(const glm::mat4& proj, float cameraX) {
        float posX = fmod(position.x, textureWidth);
        if (posX > 0) posX -= textureWidth; 

        for (int i = 0; i < 2; ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(posX + i * textureWidth, position.y, 0.0f));
            model = glm::scale(model, glm::vec3(scale, 1.0f));

            glUseProgram(shaderProgram);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "matrix"), 1, GL_FALSE, glm::value_ptr(model));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "basic_texture"), 0);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }
    }

    void setPosition(float x, float y) { position = glm::vec2(x, y); }
    void setScale(float sx, float sy) { scale = glm::vec2(sx, sy); }
    void setRotation(float angleDeg) { rotation = angleDeg; }

private:
    void initGeometry() {
        float vertices[] = {
            // Pos          // Color         // TexCoord
            -0.5f, -0.5f, 0, 1, 0, 0, 0.0f, 1.0f,
            -0.5f,  0.5f, 0, 0, 1, 0, 0.0f, 0.0f,
             0.5f,  0.5f, 0, 0, 0, 1, 1.0f, 0.0f,

            -0.5f, -0.5f, 0, 1, 0, 0, 0.0f, 1.0f,
             0.5f,  0.5f, 0, 0, 0, 1, 1.0f, 0.0f,
             0.5f, -0.5f, 0, 1, 1, 0, 1.0f, 1.0f
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
    }

    void loadTexture(const std::string& path) {
        int width, height, channels;

        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (!data) {
            std::cerr << "Erro ao carregar textura: " << path << std::endl;
            return;
        }
        textureWidth = width;

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    
};

int main() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

#pragma region Basic Setup
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "ORTHO + MOUSE + TEXTURE", nullptr, nullptr);



    if (window == nullptr) {
        std::cout << "Failed to create GLFW Window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);

    // GLAD: carrega todos os ponteiros d funções da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#pragma endregion

    const char* vertex_shader =
        "#version 400\n"
        "layout ( location = 0 ) in vec3 vPosition;"
        "layout ( location = 1 ) in vec3 vColor;"
        "layout ( location = 2 ) in vec2 vTexture;"
        "uniform mat4 proj;"
        "uniform mat4 matrix;"
        "out vec2 text_map;"
        "out vec3 color;"
        "void main() {"
        "    color = vColor;"
        "    text_map = vTexture;"
        "    gl_Position = proj * matrix * vec4 ( vPosition, 1.0);"
        "}";

    const char* fragment_shader =
        "#version 400\n"
        "in vec2 text_map;"
        "in vec3 color;"
        "uniform sampler2D basic_texture;"
        "out vec4 frag_color;"
        "void main(){"
        "   frag_color = texture(basic_texture, text_map);"
        "}";

    int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);
    int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);

    int shader_programme = glCreateProgram();
    glAttachShader(shader_programme, fs);
    glAttachShader(shader_programme, vs); 
    glLinkProgram(shader_programme);

    glm::mat4 proj = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);

    Sprite layer01(shader_programme, "../layer01_Ground.png");
    layer01.setScale(1980, HEIGHT);

    Sprite layer02(shader_programme, "../layer02_Trees.png");
    layer02.setScale(1980, HEIGHT);

    Sprite layer03(shader_programme, "../layer03_Hills_1.png");
    layer03.setScale(1980, HEIGHT);


    Sprite layer05(shader_programme, "../layer05_Clouds.png");
    layer05.setScale(1980, HEIGHT);

    Sprite layer07(shader_programme, "../layer07_Sky.png");
    layer07.setScale(1980, HEIGHT);

    Sprite plane(shader_programme, "../plane.png");
    plane.setPosition(WIDTH / 2.0f, HEIGHT / 2.0f);
    plane.setScale(150, 100);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int screenWidth, screenHeight;
        glfwGetWindowSize(window, &screenWidth, &screenHeight);
        glViewport(0, 0, screenWidth, screenHeight);

        glUseProgram(shader_programme);
        glUniformMatrix4fv(glGetUniformLocation(shader_programme, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(glGetUniformLocation(shader_programme, "matrix"), 1, GL_FALSE, glm::value_ptr(matrix));

        float speed = 200.0f * glfwGetTime(); 
        glfwSetTime(0);  

        if (pressA)
            cameraOffset.x += speed;

        if (pressD)
            cameraOffset.x -= speed;

        layer07.setPosition(WIDTH / 2.0f + cameraOffset.x * 0.1f, HEIGHT / 2.0f);
        layer05.setPosition(WIDTH / 2.0f + cameraOffset.x * 0.2f, HEIGHT / 2.0f);
        layer03.setPosition(WIDTH / 2.0f + cameraOffset.x * 0.4f, HEIGHT / 2.0f);
        layer02.setPosition(WIDTH / 2.0f + cameraOffset.x * 0.7f, HEIGHT / 2.0f);
        layer01.setPosition(WIDTH / 2.0f + cameraOffset.x * 1.0f, HEIGHT / 2.0f);

        layer07.drawTiled(proj, cameraOffset.x);
        layer05.drawTiled(proj, cameraOffset.x);
        layer03.drawTiled(proj, cameraOffset.x);
        layer02.drawTiled(proj, cameraOffset.x);
        layer01.drawTiled(proj, cameraOffset.x);
        plane.draw(proj);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_A && action == GLFW_PRESS) {
        pressA = true;
    }
    if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
        pressA = false;
        glfwSetTime(0);
    }

    if (key == GLFW_KEY_D && action == GLFW_PRESS) {
        pressD = true;
    }
    if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
        pressD = false;
        glfwSetTime(0);
    }
}
