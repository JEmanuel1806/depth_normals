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

	bool show_normals = false;
	bool show_depth_only = false;

	
	
private:
	Camera* camera;
	Shader* shader_depth;
	Shader* shader_normal;
	Shader* shader_visualize;
	Shader* shader_lines;
	GLuint VAO, VBO; 
	GLuint quadVAO;
	GLuint lineVAO;

	void configureFBO();
	GLuint setupBufferVAO();
	GLuint setupLineVAO();
};