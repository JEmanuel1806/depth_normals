#pragma once

#include <GL/glew.h>


#include "App.h"
#include "Shader.h" 
#include "Camera.h"
#include "PLY_loader.h"

class Renderer {

public:
	Renderer(Camera* camera);
	~Renderer();

	size_t points_amount;

	void start();
	void render();

private:
	Camera* camera;
	Shader* shader;
	GLuint VAO, VBO; 
};