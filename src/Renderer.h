#pragma once

#include <GL/glew.h>

#include "Shader.h" 

class Renderer {

public:
	Renderer();
	~Renderer();

	void start();
	void render();

private:
	Shader* shader;
	GLuint VAO, VBO; 
};