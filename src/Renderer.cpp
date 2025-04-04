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
	m_pCamera = cam;
	m_pShaderDepth = nullptr;
	m_pShaderPointsOnly = nullptr;
	m_pShaderCalcNormal = nullptr;
	m_pShaderPointsNormals = nullptr;
	m_VAO = 0;
	m_VBO = 0;
}

Renderer::~Renderer() {
	delete m_pShaderDepth;
	delete m_pShaderPointsOnly;
	delete m_pShaderCalcNormal;
	delete m_pShaderPointsNormals;
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
}

/* -------------------------------------------------------------------------
 * Method: start
 *
 * Initializes shaders, loads point cloud data, uploads it to the GPU,
 * configures vertex attributes and framebuffers.
 * -------------------------------------------------------------------------
 */
void Renderer::Start() {
	// Load and compile shaders for various render passes
	m_pShaderDepth = new Shader("src/shaders/depth_pass.vert", "src/shaders/depth_pass.frag");
	m_pShaderPointsOnly = new Shader("src/shaders/draw_points.vert", "src/shaders/draw_points.frag");
	m_pShaderCalcNormal = new Shader("src/shaders/calc_normal.vert", "src/shaders/calc_normal.frag");
	m_pShaderPointsNormals = new Shader("src/shaders/draw_lines.vert", "src/shaders/draw_lines.geom", "src/shaders/draw_lines.frag");

	// Load point cloud from PLY file
	PLY_loader ply_loader;
	m_pointCloud = ply_loader.LoadPLY("data/sphere_no_normals.ply");
	m_pointsAmount = m_pointCloud.PointsAmount();

	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);

	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, m_pointsAmount * sizeof(Point), m_pointCloud.m_points.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, m_position));
	glEnableVertexAttribArray(0);

	glVertexAttribIPointer(1, 1, GL_INT, sizeof(Point), (void*)offsetof(Point, m_pointID));
	glEnableVertexAttribArray(1);

	std::cout << "Rendering " << m_pointsAmount << " points.\n";
	std::cout << "sizeof(Point): " << sizeof(Point) << std::endl;

	m_lineVAO = SetupLineVAO();
	ConfigureFBO();

	GLint currentFB;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFB);
	std::cout << "Current framebuffer: " << currentFB << std::endl;

	if (m_pointCloud.m_hasNormals) {
		std::cout << "Normals detected. Skip normal calculation..." << std::endl;
	}
	else {
		std::cout << "No normals detected. Calculating normals..." << std::endl;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

/* -------------------------------------------------------------------------
 * Method: render
 *
 * Executes all render passes: depth, normal calculation (if needed),
 * and final point cloud visualization, either with or without normals.
 * 
 * -------------------------------------------------------------------------
 */
void Renderer::Render() {
	glm::mat4 view = m_pCamera->GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(m_pCamera->m_zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

	// Clear ID texture (used to map screen pixels back to point IDs), default value "-1"
	GLint clearValue = -1;
	glClearTexImage(m_idTex, 0, GL_RED_INTEGER, GL_INT, &clearValue);

	if (!m_pointCloud.m_hasNormals) {
		
		// First pass: render point cloud to fill depth and ID textures
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		m_pShaderDepth->Use();
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderDepth->m_shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderDepth->m_shaderID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(m_VAO);
		glDrawArrays(GL_POINTS, 0, m_pointsAmount);
		glBindVertexArray(0);

		// Second pass: compute normals from depth buffer, download data and store it in the pointcloud
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glDisable(GL_DEPTH_TEST);

		m_pShaderCalcNormal->Use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_depthTex);
		glUniform1i(glGetUniformLocation(m_pShaderCalcNormal->m_shaderID, "depthTex"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_idTex);
		glUniform1i(glGetUniformLocation(m_pShaderCalcNormal->m_shaderID, "idTex"), 1);

		glUniform2f(glGetUniformLocation(m_pShaderCalcNormal->m_shaderID, "iResolution"), SCREEN_WIDTH, SCREEN_HEIGHT);
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderCalcNormal->m_shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderCalcNormal->m_shaderID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderCalcNormal->m_shaderID, "invProj"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));

		glBindVertexArray(m_quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		// Copy computed normals back to point cloud
		AssignNormalsToPointCloud(m_pointCloud);

		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, m_pointsAmount * sizeof(Point), m_pointCloud.m_points.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	// Final pass: visualize the point cloud with or without normals, press N to switch
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	if (m_showNormals) {
		m_pShaderPointsNormals->Use();
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsNormals->m_shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsNormals->m_shaderID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));
	}
	else {
		m_pShaderPointsOnly->Use();
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));
	}

	glBindVertexArray(m_lineVAO);
	glDrawArrays(GL_POINTS, 0, m_pointsAmount);
	glBindVertexArray(0);
}

// VAO for the normal lines
GLuint Renderer::SetupLineVAO()
{
	glGenVertexArrays(1, &m_lineVAO);
	glBindVertexArray(m_lineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, m_position));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, m_normal));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	return m_lineVAO;
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

void Renderer::ReadNormalTexture(std::vector<glm::vec3>& normals)
{
	glBindTexture(GL_TEXTURE_2D, m_normalTex);
	normals.resize(SCREEN_WIDTH * SCREEN_HEIGHT);

	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, normals.data());

	glBindTexture(GL_TEXTURE_2D, 0);

	//std::cout << "Downloaded normals. Amount: " << normals.size() << std::endl;
}

void Renderer::ReadIDTexture(std::vector<int>& ids)
{
	glBindTexture(GL_TEXTURE_2D, m_idTex);
	ids.resize(SCREEN_WIDTH * SCREEN_HEIGHT);

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

void Renderer::AssignNormalsToPointCloud(PointCloud& pointCloud)
{
	std::vector<glm::vec3> normals;
	std::vector<int> ids;

	ReadNormalTexture(normals);
	ReadIDTexture(ids);

	for (size_t i = 0; i < normals.size(); ++i) {
		int id = ids[i];
		//std::cout << normals[i].z << std::endl;
		// std::cout << ids[i] << " - ";
		if (id >= 0 && id < pointCloud.PointsAmount()) {
			pointCloud.GetPointByID(id)->m_normal = normals[i];
			
		}
	}
}

/* -------------------------------------------------------------------------
 * configureFBO
 *
 * Creating three textures (Depth, ID, Normal) to render to with custom FBO
 * -------------------------------------------------------------------------
 */

void Renderer::ConfigureFBO()
{
	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);


	// depth tex
	glGenTextures(1, &m_depthTex);
	glBindTexture(GL_TEXTURE_2D, m_depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTex, 0);

	// ID texture, for storing IDs for each Point of the Pointcloud
	glGenTextures(1, &m_idTex);
	glBindTexture(GL_TEXTURE_2D, m_idTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RED_INTEGER, GL_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_idTex, 0);

	// normal map
	glGenTextures(1, &m_normalTex);
	glBindTexture(GL_TEXTURE_2D, m_normalTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_normalTex, 0);

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


