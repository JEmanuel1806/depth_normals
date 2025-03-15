#pragma once

#include <GL/glew.h>

#include "Shader.h" 
#include "PLY_loader.h"

class Renderer {

public:
	Renderer();
	~Renderer();

	size_t points_amount;

	void start();
	void render();

private:
	Shader* shader;
	GLuint VAO, VBO; 
};