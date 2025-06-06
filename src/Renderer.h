#pragma  once

#include "Camera.h"
#include "Shader.h" 
#include "PLY_loader.h"

#define  STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"




class    Renderer {
public:
         Renderer(Camera* pCamera);
         ~Renderer();

         void Start(std::string ply_path);
         void Render(float width, float height,float fps);

         bool m_showNormals = false;
         bool m_showDepthOnly = false;
         bool m_spinPointCloudRight = false;
         bool m_spinPointCloudLeft = false;

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
         GLuint SetupQuadVAO();

         float angle;

         void ReadNormalTexture(std::vector<glm::vec3>& normals);
         void ReadIDTexture(std::vector<int>& ids);
         void AssignNormalsToPointCloud(PointCloud& pointCloud);
         void RenderText(unsigned width, unsigned heigth, float fps);
};