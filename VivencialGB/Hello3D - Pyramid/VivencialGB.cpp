#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <fstream>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "gl_utils.h"
#include "TileMap.h"
#include "DiamondView.h"
#include "SlideView.h"
#include "ltMath.h"
#include <unordered_set>

#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

TileMap* readMap(const char* filename);
TileMap* initializeTileMap(TilemapView*& tview, GLuint& terrainTexId, const char* mapFile, const char* tilesetFile);
void loadTexture(unsigned int& texture, const char* filename);

class Player {
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

	Player(GLuint shader, const std::string& path, float scaleX = 0.25f, float scaleY = 0.25f)
		: shader(shader), position(0.0f, 0.0f), fw(0.25f), fh(0.25f) {
		initGeometry(scaleX, scaleY);
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
		glUniform1f(glGetUniformLocation(shader, "layer_z"), 0.1f);  
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	void setDirection(int dir) { acao = dir; }
	void move(float dx, float dy) { position += glm::vec2(dx, dy); }
	void setPosition(float x, float y) { position = glm::vec2(x, y); }
	void setScale(float sx, float sy) {} // opcional, ignorado no uso atual

private:
	void initGeometry(float scaleX, float scaleY) {
		float vertices[] = {
			 0.5f * scaleX,  0.5f * scaleY, 1.0f, 1.0f,
			 0.5f * scaleX, -0.5f * scaleY, 1.0f, 0.0f,
			-0.5f * scaleX, -0.5f * scaleY, 0.0f, 0.0f,
			-0.5f * scaleX,  0.5f * scaleY, 0.0f, 1.0f
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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

class Coin {
public:
	glm::vec2 position;

	Coin(float x, float y) : position(x, y) {}
};

using namespace std;
std::vector<std::vector<bool>> selectedTiles;
std::unordered_set<int> nonWalkableTiles = { 11 };
std::unordered_set<int> deadlyTiles = { 2 };
std::vector<Coin> coins;

int g_gl_width = 800;
int g_gl_height = 800;
float xi = -1.0f;
float xf = 1.0f;
float yi = -1.0f;
float yf = 1.0f;
float w = xf - xi;
float h = yf - yi;
float tw, th, tw2, th2;
int tileSetCols = 4;
int tileSetRows = 3;
float tileW, tileW2;
float tileH, tileH2;
int cx = -1, cy = -1;

TilemapView* tview = new DiamondView();
TileMap* tmap = NULL;

GLFWwindow* g_window = NULL;

TileMap* readMap(const char* filename) {
	ifstream arq(filename);
	int w, h;
	arq >> w >> h;
	TileMap* tmap = new TileMap(w, h, 0);
	for (int r = 0; r < h; r++) {
		for (int c = 0; c < w; c++) {
			int tid;
			arq >> tid;
			cout << tid << " ";
			tmap->setTile(c, r, tid);
		}
		cout << endl;
	}
	arq.close();
	return tmap;
}

void loadTexture(unsigned int& texture, const char* filename)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	// set the maximum!
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

	int width, height, nrChannels;

	unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
	if (data)
	{
		if (nrChannels == 4)
		{
			cout << "Alpha channel" << endl;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			cout << "Without Alpha channel" << endl;
			glTexImage2D(0x0DE1, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		return;
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
}

TileMap* initializeTileMap(TilemapView*& tview, GLuint& terrainTexId, const char* mapFile, const char* tilesetFile) {
	tview = new DiamondView(); 

	TileMap* tmap = readMap(mapFile);

	tw = w / (float)tmap->getWidth();
	th = tw / 2.0f;
	tw2 = th;
	th2 = th / 2.0f;
	tileW = 1.0f / (float)tileSetCols;
	tileW2 = tileW / 2.0f;
	tileH = 1.0f / (float)tileSetRows;
	tileH2 = tileH / 2.0f;

	loadTexture(terrainTexId, tilesetFile);
	tmap->setTid(terrainTexId);

	return tmap;
}

GLuint setupGeometry() {
	float vertices[] = {
		// positions   // texture coords
		xi    , yi + th2, 0.0f, tileH2,   // left
		xi + tw2, yi    , tileW2, 0.0f,   // bottom
		xi + tw , yi + th2, tileW, tileH2,  // right
		xi + tw2, yi + th , tileW2, tileH,  // top
	};
	unsigned int indices[] = {
		0, 1, 3, // first triangle
		3, 1, 2  // second triangle
	};

	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	return VAO;
}

GLuint setupMapShader() {
	char vertex_shader[1024 * 256];
	char fragment_shader[1024 * 256];
	parse_file_into_str("_geral_vs.glsl", vertex_shader, 1024 * 256);
	parse_file_into_str("_geral_fs.glsl", fragment_shader, 1024 * 256);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* p = (const GLchar*)vertex_shader;
	glShaderSource(vs, 1, &p, NULL);
	glCompileShader(vs);

	// check for compile errors
	int params = -1;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", vs);
		print_shader_info_log(vs);
		return 1; // or exit or something
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	p = (const GLchar*)fragment_shader;
	glShaderSource(fs, 1, &p, NULL);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", fs);
		print_shader_info_log(fs);
		return 1; 
	}

	GLuint shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);

	glGetProgramiv(shader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: could not link shader programme GL index %i\n",
			shader_programme);
		return 1;
	}
	glDeleteShader(vs);
	glDeleteShader(fs);

	return shader_programme;
}

GLuint compileSpriteShader() {
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

	glDeleteShader(vs);
	glDeleteShader(fs);

	return shader_programme;
}

void drawMap(GLuint shader_programme, GLuint VAO) {
	tmap->setZ(0.2f);
		
	glBindVertexArray(VAO);
	float x, y;
	int r = 0, c = 0;
	for (int r = 0; r < tmap->getHeight(); r++) {
		for (int c = 0; c < tmap->getWidth(); c++) {
			int t_id = (int)tmap->getTile(c, r);
			int u = t_id % tileSetCols;
			int v = t_id / tileSetCols;

			tview->computeDrawPosition(c, r, tw, th, x, y);

			glUniform1f(glGetUniformLocation(shader_programme, "offsetx"), u * tileW);
			glUniform1f(glGetUniformLocation(shader_programme, "offsety"), v * tileH);
			glUniform1f(glGetUniformLocation(shader_programme, "tx"), x);
			glUniform1f(glGetUniformLocation(shader_programme, "ty"), y + 1.0);
			glUniform1f(glGetUniformLocation(shader_programme, "layer_z"), tmap->getZ());
			glUniform1f(glGetUniformLocation(shader_programme, "weight"), selectedTiles[r][c] ? 0.5f : 0.0f);

			glBindTexture(GL_TEXTURE_2D, tmap->getTileSet());
			glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
	}
}


bool isWalkable(int tileID) {
	return nonWalkableTiles.find(tileID) == nonWalkableTiles.end();
}

bool isDeadly(int tileID) {
	return deadlyTiles.find(tileID) != deadlyTiles.end();
}

bool getTileCoordAt(float x, float y, int& col, int& row) {
	tview->computeMouseMap(col, row, tw, th, x, y);
	col += (tmap->getWidth() - 1) / 2;
	row += (tmap->getHeight() - 1) / 2;

	if (col < 0 || col >= tmap->getWidth() || row < 0 || row >= tmap->getHeight())
		return false;

	return true;
}

bool canMoveTo(float newX, float newY) {
	int col, row;
	if (!getTileCoordAt(newX, newY, col, row)) return false;

	int tileID = tmap->getTile(col, row);
	return isWalkable(tileID);
}

auto addCoinAtTile = [&](int col, int row) {
	float cx, cy;
	tview->computeDrawPosition(col, row, tw, th, cx, cy);
	cx += xi;
	coins.emplace_back(cx, cy);
	};


int main()
{
	restart_gl_log();
	start_gl();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	GLuint terrainTexId;
	tmap = initializeTileMap(tview, terrainTexId, "terrain1.tmap", "tiles.png");
	selectedTiles.resize(tmap->getHeight(), std::vector<bool>(tmap->getWidth(), false));

	GLuint VAO = setupGeometry();
	GLuint shader_programme = setupMapShader();

	GLuint sprite_shader_programme = compileSpriteShader();
	Player* player = new Player(sprite_shader_programme, "sully.png", 0.15f, 0.15f);

	Player* torradaSprite = new Player(sprite_shader_programme, "Gold_Coin.png", 0.10f, 0.10f);

	addCoinAtTile(3, 4);
	addCoinAtTile(7, 6);
	addCoinAtTile(12, 6);

	float startX, startY;
	tview->computeDrawPosition(2, 5, tw, th, startX, startY);
	startX += xi;
	player->setPosition(startX, startY);

	float previous = glfwGetTime();

	for (int r = 0; r < tmap->getHeight(); r++) {
		for (int c = 0; c < tmap->getWidth(); c++) {
			unsigned char t_id = tmap->getTile(c, r);
			cout << ((int)t_id) << " ";
		}
		cout << endl;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	while (!glfwWindowShouldClose(g_window))
	{
		_update_fps_counter(g_window);
		double current_seconds = glfwGetTime();

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glViewport(0, 0, g_gl_width, g_gl_height);


		float dx = 0.0f, dy = 0.0f;
		bool moving = false;

		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_W) || GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_UP)) {
			player->setDirection(3);
			dy = 0.01f;
			moving = true;
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_D) || GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_RIGHT)) {
			player->setDirection(2);
			dx = 0.01f;
			moving = true;
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_A) || GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_LEFT)) {
			player->setDirection(1);
			dx = -0.01f;
			moving = true;
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_S) || GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_DOWN)) {
			player->setDirection(0);
			dy = -0.01f;
			moving = true;
		}

		if (moving) {
			float newX = player->position.x + dx;
			float newY = player->position.y + dy;

			if (canMoveTo(newX, newY)) {
				player->move(dx, dy);

				int col, row;
				if (getTileCoordAt(player->position.x, player->position.y, col, row)) {
					selectedTiles[row][col] = true;

					int tileID = tmap->getTile(col, row);
					if (isDeadly(tileID)) {
						std::cout << "Você morreu ao pisar na Lava!" << std::endl;
						glfwSetWindowShouldClose(g_window, GL_TRUE);
					}
				}
			}

			else {
				moving = false; 
			}
		}

		player->update(current_seconds, moving);
		player->draw();

		for (const auto& coin : coins) {
			torradaSprite->setPosition(coin.position.x, coin.position.y);
			torradaSprite->draw();
		}

		for (auto it = coins.begin(); it != coins.end(); ) {
			float dist = glm::distance(player->position, it->position);
			if (dist < 0.05f) {
				std::cout << "Torrada Coletada!" << std::endl;
				it = coins.erase(it);
			}
			else {
				++it;
			}
		}

		if (coins.empty()) {
			std::cout << "Você venceu! Todas as torradas foram coletadas.\n";
			glfwSetWindowShouldClose(g_window, GL_TRUE);
		}

		glUseProgram(shader_programme);
		drawMap(shader_programme, VAO);

		glfwPollEvents();
		glfwSwapBuffers(g_window);
	}

	glfwTerminate();
	delete tmap;
	return 0;
}
