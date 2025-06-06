/* -------------------------------------------------------------------------
 *  Renderer.cpp
 *
 *  implementation of the Renderer class responsible
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
void Renderer::Start(std::string ply_path) {
	// Load and compile shaders for various render passes
	m_pShaderDepth = new Shader("src/shaders/depth_pass.vert", "src/shaders/depth_pass.frag");
	m_pShaderPointsOnly = new Shader("src/shaders/draw_points.vert", "src/shaders/draw_points.frag");
	m_pShaderCalcNormal = new Shader("src/shaders/calc_normal.vert", "src/shaders/calc_normal.frag");
	m_pShaderPointsNormals = new Shader("src/shaders/draw_lines.vert", "src/shaders/draw_lines.geom", "src/shaders/draw_lines.frag");

	// Load point cloud from PLY file
	PLY_loader ply_loader;
	m_pointCloud = ply_loader.LoadPLY(ply_path);
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
	m_quadVAO = SetupQuadVAO();
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
 *
 * Executes all render passes: depth, normal calculation (if needed),
 * and final point cloud visualization, either with or without normals.
 *
 * -------------------------------------------------------------------------
 */
void Renderer::Render(float width, float height, float fps) {
	glm::mat4 view = m_pCamera->GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(m_pCamera->m_zoom), width / height, 0.1f, 100.0f);

	if (m_spinPointCloudLeft) {
		angle = angle - 0.05f;
	}
	else if (m_spinPointCloudRight) {
		angle = angle + 0.05f;
	}
	else {
		angle = 0.0f;
	}

	glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0, 1.0, 0.0));

	// Clear ID texture (used to map screen pixels back to point IDs), default value "-1"
	GLint clearValue = -1;
	glClearTexImage(m_idTex, 0, GL_RED_INTEGER, GL_INT, &clearValue);

	if (!m_pointCloud.m_hasNormals) {

		m_pointCloud.m_hasNormals = false;
		std::cout << "Calculating.." << std::endl;

		// First pass: render point cloud to fill depth and ID textures
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		m_pShaderDepth->Use(); // use depth_pass shader
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderDepth->m_shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderDepth->m_shaderID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderDepth->m_shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));

		glBindVertexArray(m_VAO);
		glDrawArrays(GL_POINTS, 0, m_pointsAmount);
		glBindVertexArray(0);

		// Second pass: compute normals from depth buffer, download data and store it in the pointcloud

		glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

		m_pShaderCalcNormal->Use(); // use calc_normal shader
		

		GLenum attachments[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, attachments);

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
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_POINT_SMOOTH);

	if (m_showNormals) {
		// draw white points
		m_pShaderPointsOnly->Use();
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glBindVertexArray(m_lineVAO);
		glDrawArrays(GL_POINTS, 0, m_pointsAmount);

		// draw normal lines
		m_pShaderPointsNormals->Use();
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsNormals->m_shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsNormals->m_shaderID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsNormals->m_shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glBindVertexArray(m_lineVAO);
		glDrawArrays(GL_POINTS, 0, m_pointsAmount);
	}
	else {
		m_pShaderPointsOnly->Use();
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glBindVertexArray(m_lineVAO);
		glDrawArrays(GL_POINTS, 0, m_pointsAmount);
	}


	glBindVertexArray(m_lineVAO);
	glDrawArrays(GL_POINTS, 0, m_pointsAmount);
	glBindVertexArray(0);

	RenderText(width, height, fps);
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

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, m_color));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	return m_lineVAO;
}

// VAO for the normal lines
GLuint Renderer::SetupQuadVAO()
{
	float quadVertices[] = {
		// positions    // texCoords
		-1.0f,  1.0f,    0.0f, 1.0f,
		-1.0f, -1.0f,    0.0f, 0.0f,
		 1.0f, -1.0f,    1.0f, 0.0f,

		-1.0f,  1.0f,    0.0f, 1.0f,
		 1.0f, -1.0f,    1.0f, 0.0f,
		 1.0f,  1.0f,    1.0f, 1.0f
	};

	GLuint quadVBO;
	glGenVertexArrays(1, &m_quadVAO);
	glGenBuffers(1, &quadVBO);

	glBindVertexArray(m_quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	return m_quadVAO;
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


		/*
		if (normals[i].x != 0)
			std::cout << "normal" << i << ":" << normals[i].x << "//" << normals[i].y << "//" << normals[i].z << "\n";
		*/

		if (id >= 0 && id < pointCloud.PointsAmount()) {
			pointCloud.GetPointByID(id)->m_normal = normals[i];
		}
	}


	// DEBUG
	/*


	for (int i = 0; i < normals.size(); i++)
	{
		int id = ids[i];

		if (id >= 0 && id < pointCloud.PointsAmount()) {
			pointCloud.GetPointByID(id)->m_normal = normals[i];
		}
	}
	*/



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

	// normal map
	glGenTextures(1, &m_normalTex);
	glBindTexture(GL_TEXTURE_2D, m_normalTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_normalTex, 0);

	// ID texture, for storing IDs for each Point of the Pointcloud
	glGenTextures(1, &m_idTex);
	glBindTexture(GL_TEXTURE_2D, m_idTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RED_INTEGER, GL_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_idTex, 0);



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

/* -------------------------------------------------------------------------
 *
 * Helper function to render some additional information in form of text
 * FPS
 * Amount of points
 * Normal information and deviation
 *
 * -------------------------------------------------------------------------
 */
void Renderer::RenderText(unsigned width, unsigned height, float fps)
{

	glUseProgram(0);

	// Set up orthographic projection for 2D screen-space rendering (e.g., text)
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, height, 0, -1, 1);

	std::stringstream ss;

	ss << "FPS: " << fps << "\nPoints: " << m_pointsAmount;
	std::string text = ss.str();

	static char buffer[99999];
	int num_quads = stb_easy_font_print(20, 20, (char*)text.c_str(), NULL, buffer, sizeof(buffer));

	glColor3f(0.0f, 1.0f, 0.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 16, buffer);
	glDrawArrays(GL_QUADS, 0, num_quads * 4);
	glDisableClientState(GL_VERTEX_ARRAY);
}


