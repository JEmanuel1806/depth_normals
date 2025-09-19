#pragma  once

#include "Camera.h"
#include "Shader.h" 
#include "PLY_loader.h"
#include "CommandLine.h"

#define  STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

class    Renderer {
public:
         Renderer(Camera* pCamera);
         ~Renderer();

         void Start(std::string ply_path, unsigned int width, unsigned int height);
         void Render(float fps);

         GLuint qTotal, qRef, qAcc, qFin, qSplat , qReadBack, t0, t1; //performance query metrics

         struct NormalStats {
             GLuint occludedNrml;
             GLuint goodNrml;
             GLuint mediumNrml;
             GLuint badNrml;
         };


         bool m_showNormals = false;
         bool m_showPoints = true;
         bool m_showMesh = false;
         bool m_showMeshIPSR = false;
         bool m_showDepthOnly = false;
         bool m_recalculate = true;
         bool m_showIDMap = false;
         bool m_showAABB = false;
         bool m_spinPointCloudRight = false;
         bool m_spinPointCloudLeft = false;
         bool saveToPLY = false;
         bool automatic_mode = true;

         size_t cameraViewPos = 0;

         enum class DisplayMode { POINTCLOUD, IPSR_MESH, POISSON_MESH };
         DisplayMode m_displayMode = DisplayMode::POINTCLOUD;

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

         float goodNormal = 10.0f; //threshold for a normal to be good (e.g. 10 degrees of difference)
         float badNormal = 30.0f;  //same for bad (red)

         glm::vec3 lightPos = glm::vec3(3.0f, 2.0f, 3.0f);
         float lightYaw = 0.0f;
         float lightPitch = 0.0f;

         PLY_loader plyLoader;
         CommandLine cmd;
         NormalStats m_stats;

private:
         Camera* m_pCamera = nullptr;

         PointCloud m_pointCloud;
         PointCloud m_pointCloudGT; // ground truth
         PointCloud m_meshIPSR;
         PointCloud m_meshPoisson;
         GLuint m_meshVAO_IPSR = 0;
         GLuint m_meshVAO_Poisson = 0;
         GLuint m_meshIndexCount_IPSR = 0;
         GLuint m_meshIndexCount_Poisson = 0;

         unsigned int m_height;
         unsigned int m_width;

         struct BoundingBox {
             glm::vec3 min;
             glm::vec3 max;

             glm::vec3 center() const {
                 return (min + max) * 0.5f;
             }

             glm::vec3 extent() const {
                 return (max - min) * 0.5f;
             }

             glm::vec3 size() const {
                 return (max - min);
             }
         };

         BoundingBox aabb;

         Shader* m_pShaderDepth = nullptr;
         Shader* m_pShaderBigSplats = nullptr;
         Shader* m_pShaderPointsOnly = nullptr;
         Shader* m_pShaderMesh = nullptr;
         Shader* m_pShaderCalcNormal = nullptr;
         Shader* m_pShaderNormalAvg = nullptr;
         Shader* m_pShaderEvaluateNormal = nullptr;
         Shader* m_pShaderNormalCompute = nullptr;
         Shader* m_pShaderPointsNormals = nullptr;
         Shader* m_pDebugTexture = nullptr;
         Shader* m_pDebugNormalTexture = nullptr;
         Shader* m_pDrawAABB = nullptr;

         GLuint m_VAO = 0;
         GLuint m_VBO = 0;
         GLuint m_quadVAO = 0;
         GLuint m_lineVAO = 0;
         GLuint m_AABO_VAO = 0;
         GLuint m_pointNormalSSBO;
         GLuint m_pointGTSSBO;
         GLuint m_pointAvgSSBO;
         GLuint m_statsSSBO;

private:
         void ConfigureNormalSSBO();
         void ConfigureGTSSBO();
         void ConfigureAvgSSBO();
         void ConfigureRefFBO();
         void ConfigureSplatFBO();
         void ConfigureStatsSSBO();
         void ConfigureFBO(GLuint& fbo, GLuint& depthTex, GLuint& idTex);
         GLuint SetupLineVAO();
         GLuint SetupQuadVAO();
         GLuint SetupMeshVAO(const PointCloud &pc);
         GLuint SetupBBoxVAO(const BoundingBox &boundingBox);

         // Render Loop
         void ComputeNormalsForView(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& model);
         void ComputeNormalStatsGPU(float goodDeg, float badDeg);
         void ComputeMeshNormals(PointCloud& mesh);

         BoundingBox CalcAABB(PointCloud &pointcloud);
         void RenderText(float fps, PointCloud pc, PointCloud pcGT);

         float angle;

};