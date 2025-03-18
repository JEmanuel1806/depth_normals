#pragma once

#include "Camera.h"
#include "Shader.h" 
#include "PLY_loader.h"



class Renderer {

public:
	Renderer(Camera* camera);
	~Renderer();

	GLuint FBO;
	GLuint depthTex;
	GLuint normalTex;

	size_t points_amount;

	void start();
	void render();

	
	
private:
	Camera* camera;
	Shader* shader_depth;
	Shader* shader_normal;
	Shader* shader_visualize;
	GLuint VAO, VBO; 
	GLuint quadVAO;

	void configureFBO();
	GLuint setupBufferVAO();
};