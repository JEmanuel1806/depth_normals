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
	GLuint idTex;
	GLuint normalTex;

	size_t points_amount;

	void start();
	void render();

	bool show_normals = false;
	bool show_depth_only = false;
	
private:
	Camera* camera;
	PointCloud pointCloud;
	Shader* shader_depth;
	Shader* shader_points_only;
	Shader* shader_calc_normal;
	Shader* shader_points_normals;
	GLuint VAO, VBO; 
	GLuint quadVAO;
	GLuint lineVAO;

	void configureFBO();
	GLuint setupBufferVAO();
	GLuint setupLineVAO();

	void readNormalTex(std::vector<glm::vec3>& normals);
	void readIDTex(std::vector<int>& ids);
	void assignNormalsToPointCloud(PointCloud& pointCloud);
};