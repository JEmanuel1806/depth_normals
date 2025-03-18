#include "Renderer.h"


float quadVertices[] = {
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	-1.0f,  1.0f,  0.0f, 1.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
};

Renderer::Renderer(Camera* cam) {
    camera = cam;
    shader_depth = nullptr;
	shader_normal = nullptr;
	shader_visualize = nullptr;
    VAO = 0;
    VBO = 0;
}

Renderer::~Renderer() {
	delete shader_depth;
	delete shader_normal;
	delete shader_visualize;
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

void Renderer::start() {

    shader_depth = new Shader("src/shaders/depth_pass.vert","src/shaders/depth_pass.frag");
	shader_normal = new Shader("src/shaders/normal_pass.vert","src/shaders/normal_pass.frag");
	shader_visualize = new Shader("src/shaders/visualize_normal.vert","src/shaders/visualize_normal.frag");
    
	PLY_loader ply_loader;

    std::vector<PointBuffer> points = ply_loader.load_ply("data/bimba.ply");
    points_amount = points.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(PointBuffer), points.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);


	quadVAO = setupBufferVAO();
	configureFBO();

	// Check currentFBO
	GLint currentFB;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFB);
	std::cout << "Current framebuffer: " << currentFB << std::endl;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::render()
{    
	// FIRST PASS (DEPTH TEX)
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);


	shader_depth->use();

	glm::mat4 view = camera->GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

	glUniformMatrix4fv(glGetUniformLocation(shader_depth->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(shader_depth->ID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));

	glBindVertexArray(VAO);
	glDrawArrays(GL_POINTS, 0, points_amount);
	glBindVertexArray(0);
	
	
	// SECOND PASS (NORMAL TEX)
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glDisable(GL_DEPTH_TEST);

	shader_normal->use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthTex);
	glUniform1i(glGetUniformLocation(shader_normal->ID, "depthTex"), 0);
	glUniform2f(glGetUniformLocation(shader_normal->ID, "iResolution"), SCR_WIDTH, SCR_HEIGHT);
	glUniformMatrix4fv(glGetUniformLocation(shader_normal->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(shader_normal->ID, "proj"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(shader_normal->ID, "invProj"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	
	// THIRD PASS (visualiuze for debugging)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	shader_visualize->use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, normalTex);
	glUniform1i(glGetUniformLocation(shader_visualize->ID, "normalTex"), 0);

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	
}

GLuint Renderer::setupBufferVAO() {
	GLuint quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);

	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return quadVAO;
}

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

	// normal map
	glGenTextures(1, &normalTex);
	glBindTexture(GL_TEXTURE_2D, normalTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, normalTex, 0);

	GLenum attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_COMPLETE) {
		printf("FBO complete!\n");
	}
	else {
		printf("FBO incomplete! Error: %d\n", status);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
