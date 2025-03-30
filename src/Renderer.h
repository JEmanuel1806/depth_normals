#pragma once

#include "Camera.h"
#include "Shader.h" 
#include "PLY_loader.h"



class Renderer {
public:
    Renderer(Camera* pCamera);
    ~Renderer();

    void Start();
    void Render();

    bool m_showNormals = false;
    bool m_showDepthOnly = false;

    GLuint m_fbo = 0;
    GLuint m_depthTex = 0;
    GLuint m_idTex = 0;
    GLuint m_normalTex = 0;

    size_t m_pointsAmount = 0;

private:
    Camera* m_pCamera = nullptr;

    PointCloud m_pointCloud;

    Shader* m_pShaderDepth = nullptr;
    Shader* m_pShaderPointsOnly = nullptr;
    Shader* m_pShaderCalcNormal = nullptr;
    Shader* m_pShaderPointsNormals = nullptr;

    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_quadVAO = 0;
    GLuint m_lineVAO = 0;

private:
    void ConfigureFBO();
    GLuint SetupLineVAO();

    void ReadNormalTexture(std::vector<glm::vec3>& normals);
    void ReadIDTexture(std::vector<int>& ids);
    void AssignNormalsToPointCloud(PointCloud& pointCloud);
};