#include "Renderer.h"

static const GLfloat vertices[] = {
   -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    0.0f,  0.5f, 0.0f
};

Renderer::Renderer() {
    shader = nullptr;
    VAO = 0;
    VBO = 0;
}

Renderer::~Renderer() {
	delete shader;
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

void Renderer::start() {

    shader = new Shader();
    PLY_loader ply_loader;

    std::vector<PointBuffer> points = ply_loader.load_ply("data/walrus.ply");
    points_amount = points.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(PointBuffer), points.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::render()
{
	glClear(GL_COLOR_BUFFER_BIT);
    shader->use();
	glBindVertexArray(VAO);
	glDrawArrays(GL_POINTS, 0, points_amount);
	glBindVertexArray(0);
}
