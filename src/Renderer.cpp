/* -------------------------------------------------------------------------
 *  Renderer.cpp
 *
 *  This file contains the implementation of the Renderer class responsible
 *  for loading the point cloud, performing multiple rendering passes
 *  (depth, normal calculation, and final visualization), and managing GPU buffers.
 *
 * -------------------------------------------------------------------------
 */

#include "Renderer.h"

Renderer::Renderer(Camera* cam) {
	camera = cam;
	shader_depth = nullptr;
	shader_points_only = nullptr;
	shader_calc_normal = nullptr;
	VAO = 0;
	VBO = 0;
}

Renderer::~Renderer() {
	delete shader_depth;
	delete shader_points_only;
	delete shader_calc_normal;
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

/* -------------------------------------------------------------------------
 * Method: start
 *
 * Initializes shaders, loads point cloud data, uploads it to the GPU,
 * configures vertex attributes and framebuffers.
 * -------------------------------------------------------------------------
 */
void Renderer::start() {
	// Load and compile shaders for various render passes
	shader_depth = new Shader("src/shaders/depth_pass.vert", "src/shaders/depth_pass.frag");
	shader_points_only = new Shader("src/shaders/draw_points.vert", "src/shaders/draw_points.frag");
	shader_calc_normal = new Shader("src/shaders/calc_normal.vert", "src/shaders/calc_normal.frag");
	shader_points_normals = new Shader("src/shaders/draw_lines.vert", "src/shaders/draw_lines.geom", "src/shaders/draw_lines.frag");

	// Load point cloud from PLY file
	PLY_loader ply_loader;
	pointCloud = ply_loader.load_ply("data/sphere_no_normals.ply");
	points_amount = pointCloud.points_amount();

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, points_amount * sizeof(Point), pointCloud.points.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, position));
	glEnableVertexAttribArray(0);

	glVertexAttribIPointer(1, 1, GL_INT, sizeof(Point), (void*)offsetof(Point, ID));
	glEnableVertexAttribArray(1);

	std::cout << "Rendering " << points_amount << " points.\n";
	std::cout << "sizeof(Point): " << sizeof(Point) << std::endl;

	lineVAO = setupLineVAO();
	configureFBO();

	GLint currentFB;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFB);
	std::cout << "Current framebuffer: " << currentFB << std::endl;

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

/* -------------------------------------------------------------------------
 * Method: render
 *
 * Executes all render passes: depth, normal calculation (if needed),
 * and final point cloud visualization.
 * -------------------------------------------------------------------------
 */
void Renderer::render() {
	glm::mat4 view = camera->GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

	// Clear ID texture (used to map screen pixels back to point IDs)
	GLint clearValue = -1;
	glClearTexImage(idTex, 0, GL_RED_INTEGER, GL_INT, &clearValue);

	if (!pointCloud.hasNormals) {
		// First pass: render point cloud to fill depth and ID textures
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		shader_depth->use();
		glUniformMatrix4fv(glGetUniformLocation(shader_depth->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shader_depth->ID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, points_amount);
		glBindVertexArray(0);

		// Second pass: compute normals from depth buffer
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glDisable(GL_DEPTH_TEST);

		shader_calc_normal->use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthTex);
		glUniform1i(glGetUniformLocation(shader_calc_normal->ID, "depthTex"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, idTex);
		glUniform1i(glGetUniformLocation(shader_calc_normal->ID, "idTex"), 1);

		glUniform2f(glGetUniformLocation(shader_calc_normal->ID, "iResolution"), SCR_WIDTH, SCR_HEIGHT);
		glUniformMatrix4fv(glGetUniformLocation(shader_calc_normal->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shader_calc_normal->ID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(shader_calc_normal->ID, "invProj"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		// Copy computed normals back to point cloud
		assignNormalsToPointCloud(pointCloud);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, points_amount * sizeof(Point), pointCloud.points.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	// Final pass: visualize the point cloud with or without normals, press N to switch
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	if (show_normals) {
		shader_points_normals->use();
		glUniformMatrix4fv(glGetUniformLocation(shader_points_normals->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shader_points_normals->ID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));
	}
	else {
		shader_points_only->use();
		glUniformMatrix4fv(glGetUniformLocation(shader_points_only->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shader_points_only->ID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));
	}

	glBindVertexArray(lineVAO);
	glDrawArrays(GL_POINTS, 0, points_amount);
	glBindVertexArray(0);
}

// VAO for the normal lines
GLuint Renderer::setupLineVAO()
{
	glGenVertexArrays(1, &lineVAO);
	glBindVertexArray(lineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, position));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, normal));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	return lineVAO;
}

/* -------------------------------------------------------------------------
 * Helper functions to read data from textures
 *
 * Reading the data from the generated normal texture and storing its content in a vector.
 * Also reading the ids from the helper ID texture and storing it in an additional array. 
 * Blank spots on the ID texture stay with value "-1" and can be distinguished that way
 * 
 * -------------------------------------------------------------------------
 */

void Renderer::readNormalTex(std::vector<glm::vec3>& normals)
{
	glBindTexture(GL_TEXTURE_2D, normalTex);
	normals.resize(SCR_WIDTH * SCR_HEIGHT);

	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, normals.data());

	glBindTexture(GL_TEXTURE_2D, 0);

	//std::cout << "Downloaded normals. Amount: " << normals.size() << std::endl;
}

void Renderer::readIDTex(std::vector<int>& ids)
{
	glBindTexture(GL_TEXTURE_2D, idTex);
	ids.resize(SCR_WIDTH * SCR_HEIGHT);

	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_INT, ids.data());

	glBindTexture(GL_TEXTURE_2D, 0);

}

/* -------------------------------------------------------------------------
 * assignNormalsToPointCloud
 *
 * Reading out the vector with the normal information from the texture and assigning it
 * to the Point Cloud, replacing default values from before.
 * The ID vector helps distinguish, which point represents which fragment and can thus
 * assign the correct normal to each point
 * -------------------------------------------------------------------------
 */

void Renderer::assignNormalsToPointCloud(PointCloud& pointCloud)
{
	std::vector<glm::vec3> normals;
	std::vector<int> ids;

	readNormalTex(normals);
	readIDTex(ids);

	for (size_t i = 0; i < normals.size(); ++i) {
		int id = ids[i];
		if (id >= 0 && id < pointCloud.points_amount()) {
			pointCloud.getPointByID(id)->normal = normals[i];
		}
	}
}

/* -------------------------------------------------------------------------
 * configureFBO
 *
 * Creating three textures (Depth, ID, Normal) to render to with custom FBO
 * -------------------------------------------------------------------------
 */

void Renderer::configureFBO()
{
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);


	// depth tex
	glGenTextures(1, &depthTex);
	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);

	// ID texture, for storing IDs for each Point of the Pointcloud
	glGenTextures(1, &idTex);
	glBindTexture(GL_TEXTURE_2D, idTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED_INTEGER, GL_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, idTex, 0);


	// normal map
	glGenTextures(1, &normalTex);
	glBindTexture(GL_TEXTURE_2D, normalTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, normalTex, 0);

	GLenum attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_COMPLETE) {
		printf("FBO complete!\n");
	}
	else {
		printf("FBO incomplete! Error: %d\n", status);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


