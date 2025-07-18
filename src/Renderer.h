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

         void Start(std::string ply_path, unsigned int width, unsigned int height);
         void Render(float fps);

         bool m_showNormals = false;
         bool m_showDepthOnly = false;
         bool m_recalculate = true;
         bool m_showIDMap = false;
         bool m_showNormalMap = false;
         bool m_showFrustum = false;
         bool m_spinPointCloudRight = false;
         bool m_spinPointCloudLeft = false;

         GLuint m_fbo = 0;
         GLuint m_depthTex = 0;
         GLuint m_idTex = 0;
         GLuint m_normalTex = 0;

         glm::vec3 expectedNormal;

         size_t m_pointsAmount = 0;

         float pointSize = 1.0f;
         float m_zNear = 0.1f;
         float m_zFar = 100.0f;

private:
         Camera* m_pCamera = nullptr;

         PointCloud m_pointCloud;

         unsigned int m_height;
         unsigned int m_width;

         Shader* m_pShaderDepth = nullptr;
         Shader* m_pShaderPointsOnly = nullptr;
         Shader* m_pShaderCalcNormal = nullptr;
         Shader* m_pShaderPointsNormals = nullptr;
         Shader* m_pDebugIDTexture = nullptr;
         Shader* m_pDebugNormalTexture = nullptr;
         Shader* m_pDrawFrustum = nullptr;

         GLuint m_VAO = 0;
         GLuint m_VBO = 0;
         GLuint m_quadVAO = 0;
         GLuint m_lineVAO = 0;
         GLuint m_frustumVAO = 0;

private:
         void ConfigureFBO();
         GLuint SetupLineVAO();
         GLuint SetupQuadVAO();
         GLuint SetupFrustumVAO(const glm::mat4& projection, const glm::mat4& view);

         float angle;

         void ReadNormalTexture(std::vector<glm::vec3>& normals);
         void ReadIDTexture(std::vector<int>& ids);
         void AssignNormalsToPointCloud(PointCloud& pointCloud);
         void RenderText(float fps, PointCloud pc);
};