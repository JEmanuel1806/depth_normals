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

         GLuint qTotal, qRef, qAcc, qFin, qSplat , qReadBack, t0, t1; //performance query metrics

         bool m_showNormals = false;
         bool m_showPoints = true;
         bool m_showDepthOnly = false;
         bool m_recalculate = true;
         bool m_showIDMap = false;
         bool m_showFrustum = false;
         bool m_spinPointCloudRight = false;
         bool m_spinPointCloudLeft = false;
         bool saveToPLY = false;

         GLuint m_fboRef = 0;
         GLuint m_depthTexRef = 0;
         GLuint m_idTexRef = 0;

         GLuint m_fboSplat = 0;
         GLuint m_depthTexSplat = 0;
         GLuint m_idTexSplat = 0;

         glm::vec3 expectedNormal;

         size_t m_pointsAmount = 0;
         size_t m_pointsAmountGT = 0;

         float splatSize = 3.0f;
         float m_zNear = 0.1f;
         float m_zFar = 100.0f;

         PLY_loader plyLoader;

private:
         Camera* m_pCamera = nullptr;

         PointCloud m_pointCloud;
         PointCloud m_pointCloudGT; // ground truth

         unsigned int m_height;
         unsigned int m_width;

         Shader* m_pShaderDepth = nullptr;
         Shader* m_pShaderBigSplats = nullptr;
         Shader* m_pShaderPointsOnly = nullptr;
         Shader* m_pShaderCalcNormal = nullptr;
         Shader* m_pShaderNormalAvg = nullptr;
         Shader* m_pShaderNormalCompute = nullptr;
         Shader* m_pShaderPointsNormals = nullptr;
         Shader* m_pDebugTexture = nullptr;
         Shader* m_pDebugNormalTexture = nullptr;
         Shader* m_pDrawFrustum = nullptr;

         GLuint m_VAO = 0;
         GLuint m_VBO = 0;
         GLuint m_quadVAO = 0;
         GLuint m_lineVAO = 0;
         GLuint m_frustumVAO = 0;
         GLuint m_pointNormalSSBO;
         GLuint m_pointGTSSBO;
         GLuint m_pointAvgSSBO;

private:
         void ConfigureNormalSSBO();
         void ConfigureGTSSBO();
         void ConfigureAvgSSBO();
         void ConfigureRefFBO();
         void ConfigureSplatFBO();
         void ConfigureFBO(GLuint& fbo, GLuint& depthTex, GLuint& idTex);
         GLuint SetupLineVAO();
         GLuint SetupQuadVAO();
         GLuint SetupFrustumVAO(const glm::mat4& projection, const glm::mat4& view);

         void RenderText(float fps, PointCloud pc, PointCloud pcGT);

         float angle;

};