/* -------------------------------------------------------------------------
 *  Renderer.cpp
 *
 *  implementation of the Renderer class responsible
 *  for loading the point cloud, performing multiple rendering passes
 *  (depth, normal calculation, and final visualization), and managing GPU
 * buffers.
 *
 * -------------------------------------------------------------------------
 */

#include <set>
#include <unordered_map>

#include "Renderer.h"
#include "glm/gtx/string_cast.hpp"

Renderer::Renderer(Camera* cam) {
    m_pCamera = cam;
    m_pShaderDepth = nullptr;
    m_pShaderBigSplats = nullptr;
    m_pShaderPointsOnly = nullptr;
    m_pShaderCalcNormal = nullptr;
    m_pShaderNormalAvg = nullptr;
    m_pShaderNormalCompute = nullptr;
    m_pShaderPointsNormals = nullptr;
    m_pDebugTexture = nullptr;
    m_VAO = 0;
    m_VBO = 0;
    m_lineVAO = 0;
    m_quadVAO = 0;
    m_frustumVAO = 0;
}

Renderer::~Renderer() {
    delete m_pShaderDepth;
    delete m_pShaderBigSplats;
    delete m_pShaderPointsOnly;
    delete m_pShaderCalcNormal;
    delete m_pShaderNormalAvg;
    delete m_pShaderPointsNormals;
    delete m_pShaderNormalCompute;
    delete m_pDebugTexture;
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteVertexArrays(1, &m_lineVAO);
    glDeleteVertexArrays(1, &m_quadVAO);
    glDeleteVertexArrays(1, &m_frustumVAO);
}

/* -------------------------------------------------------------------------
 * Method: start
 *
 * Initializes shaders, loads point cloud data, uploads it to the GPU,
 * configures vertex attributes and framebuffers.
 * -------------------------------------------------------------------------
 */
void Renderer::Start(std::string ply_path, unsigned int width, unsigned int height) {
    // Load and compile shaders for various render passes
    m_pShaderDepth = new Shader("src/shaders/depth_pass.vert", "src/shaders/depth_pass.frag");
    m_pShaderBigSplats = new Shader("src/shaders/biggerSplat_pass.vert", "src/shaders/biggerSplat_pass.frag");
    m_pShaderPointsOnly = new Shader("src/shaders/draw_points.vert", "src/shaders/draw_points.frag");
    m_pShaderCalcNormal = new Shader("src/shaders/calc_normal.vert", "src/shaders/calc_normal.frag");
    m_pShaderNormalCompute = new Shader("src/shaders/calc_normal.comp");
    m_pShaderNormalAvg = new Shader("src/shaders/average_normal.comp");
    m_pShaderPointsNormals = new Shader("src/shaders/draw_lines.vert", "src/shaders/draw_lines.geom",
        "src/shaders/draw_lines.frag");
    m_pDebugTexture =
        new Shader("src/shaders/debug/debug_id_tex.vert", "src/shaders/debug/debug_id_tex.frag");
    m_pDrawFrustum = new Shader("src/shaders/draw_frustum.vert", "src/shaders/draw_frustum.frag");

    m_width = width;
    m_height = height;

    // Load point cloud from PLY file
    PLY_loader ply_loader;
    m_pointCloud = ply_loader.LoadPLY(ply_path);
    m_pointsAmount = m_pointCloud.PointsAmount();

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glBufferData(GL_ARRAY_BUFFER, m_pointsAmount * sizeof(Point), m_pointCloud.m_points.data(),
        GL_STATIC_DRAW);

    glVertexAttribIPointer(0, 1, GL_INT, sizeof(Point),
        (void*)offsetof(Point, m_pointID));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point),
        (void*)offsetof(Point, m_position));
    glEnableVertexAttribArray(1);

    std::cout << "Rendering " << m_pointsAmount << " points.\n";
    std::cout << "sizeof(Point): " << sizeof(Point) << std::endl;

    m_lineVAO = SetupLineVAO();
    m_quadVAO = SetupQuadVAO();
    
    // for FRUSTUM
    glm::mat4 projection =
        glm::perspective(glm::radians(m_pCamera->m_zoom), float(m_width) / float(m_height), m_zNear, m_zFar);
    m_frustumVAO = SetupFrustumVAO(projection, m_pCamera->GetViewMatrix());
    
    ConfigureRefFBO();
    ConfigureSplatFBO();
    ConfigureAvgSSBO();
    ConfigureNormalSSBO();
    
    GLint currentFB;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFB);
    std::cout << "Current framebuffer: " << currentFB << std::endl;

    if (m_pointCloud.m_hasNormals) {
        std::cout << "Normals detected. Skip normal calculation..." << std::endl;
        expectedNormal = m_pointCloud.GetNormalByID(4);
        std::cout << "Expected Normal for ID: " << 4<< " : " << glm::to_string(expectedNormal)
            << std::endl;
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
void Renderer::Render(float fps) {
    glm::mat4 view = m_pCamera->GetViewMatrix();
    glm::mat4 projection =
        glm::perspective(glm::radians(m_pCamera->m_zoom), float(m_width) / float(m_height), m_zNear, m_zFar);

    // Just for spinning the pointcloud with arrow keys
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

    glClearColor(0.141f, 0.149f, 0.192f, 1.0f);

    // if its ground truth (point cloud with normals) dont calculate obv
    // Only calculate if "TAB" is pressed (=Recalculate on) to prevent LAG

    if (!m_pointCloud.m_hasNormals && m_recalculate) {

        // First pass: render point cloud to fill depth and ID textures (reference textures)
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboRef);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        m_pShaderDepth->Use();  // use depth_pass shader
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderDepth->m_shaderID, "view"), 1, GL_FALSE,
            glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderDepth->m_shaderID, "proj"), 1, GL_FALSE,
            glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderDepth->m_shaderID, "model"), 1, GL_FALSE,
            glm::value_ptr(model));

        glBindVertexArray(m_VAO);
        glDrawArrays(GL_POINTS, 0, m_pointsAmount);
        glBindVertexArray(0);

        // Second pass: render point cloud with bigger splats and store to 2 textures (splat textures)
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboSplat);
        //glDepthMask(GL_FALSE);
        //glDisable(GL_BLEND);

        /* only activate for debug!*/
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        
        m_pShaderBigSplats->Use();  
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderBigSplats->m_shaderID, "view"), 1, GL_FALSE,
            glm::value_ptr(view));                
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderBigSplats->m_shaderID, "proj"), 1, GL_FALSE,
            glm::value_ptr(projection));          
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderBigSplats->m_shaderID, "model"), 1, GL_FALSE,
            glm::value_ptr(model));
        glUniform1f(glGetUniformLocation(m_pShaderBigSplats->m_shaderID, "pointSize"), splatSize);
        
        glBindVertexArray(m_VAO);
        glDrawArrays(GL_POINTS, 0, m_pointsAmount);
        glBindVertexArray(0);

        // Third pass: compute normals from depth buffer, calculate in compute shader 

        glDisable(GL_DEPTH_TEST);

        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
        glUseProgram(m_pShaderNormalCompute->m_shaderID);

        // reference textures

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_depthTexRef);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glUniform1i(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "ref_depth"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_idTexRef);
        glUniform1i(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "ref_id"), 1);

        // splat textures

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_depthTexSplat);
        glUniform1i(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "splat_depth"), 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, m_idTexSplat);
        glUniform1i(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "splat_id"), 3);

        // other uniforms

        glUniform2i(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "screenSize"), m_width, m_height);
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "view"), 1, GL_FALSE,
            glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "InvView"), 1, GL_FALSE,
            glm::value_ptr(glm::inverse(view)));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "proj"), 1, GL_FALSE,
            glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "invProj"), 1,
            GL_FALSE, glm::value_ptr(glm::inverse(projection)));
        glUniform1f(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "zNear"), m_zNear);
        glUniform1f(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "zFar"), m_zFar);
        glUniform1f(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "maxID"), m_pointsAmount);


        // compute shader vars

        GLuint workGroupX = (m_width + 7) / 8;
        GLuint workGroupY = (m_height + 7) / 8;
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_pointNormalSSBO);

        glDispatchCompute(workGroupX, workGroupY, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        std::cout << "-------------(Re)calculating normals for " << m_pointsAmount << " points.-----------------" << std::endl;

        // Fourth Pass: Average the accumulated normals from pass before

        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
        glUseProgram(m_pShaderNormalAvg->m_shaderID);

        // reference textures

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_depthTexRef);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glUniform1i(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "ref_depth"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_idTexRef);
        glUniform1i(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "ref_id"), 1);

        // splat textures

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_depthTexSplat);
        glUniform1i(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "splat_depth"), 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, m_idTexSplat);
        glUniform1i(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "splat_id"), 3);

        // other uniforms

        glUniform2i(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "screenSize"), m_width, m_height);
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "view"), 1, GL_FALSE,
            glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "ínvView"), 1, GL_FALSE,
            glm::value_ptr(glm::inverse(view)));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "proj"), 1, GL_FALSE,
            glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "invProj"), 1,
            GL_FALSE, glm::value_ptr(glm::inverse(projection)));
        glUniform1f(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "zNear"), m_zNear);
        glUniform1f(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "zFar"), m_zFar);
        glUniform1f(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "maxID"), m_pointsAmount);

        // compute shader vars

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_pointNormalSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_pointAvgSSBO);

        glDispatchCompute(workGroupX, workGroupY, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        std::cout << "-------------(Re)calculating normals for " << m_pointsAmount << " points.-----------------" << std::endl;

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointAvgSSBO);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Point) * m_pointsAmount, m_pointCloud.m_points.data());

        // back to VBO for arrow vis
        glBindBuffer(GL_COPY_READ_BUFFER, m_pointAvgSSBO);
        glBindBuffer(GL_COPY_WRITE_BUFFER, m_VBO);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(Point) * m_pointsAmount);
   
        Point p = m_pointCloud.m_points[4];
        std::cout << "Point ID: " << p.m_pointID << std::endl;
        std::cout << "Position: " << p.m_position.x << ", " << p.m_position.y << ", " << p.m_position.z << std::endl;
        std::cout << "Normal: " << p.m_normal.x << ", " << p.m_normal.y << ", " << p.m_normal.z << std::endl;
        std::cout << "sizeof(Point) = " << sizeof(Point) << std::endl;
        m_pCamera->HasChanged = false;

        // m_pointCloud.m_hasNormals = true;
    }

    // Final pass: visualize the point cloud with or without normals, press N to
    // switch
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SMOOTH);

    // for debugging any texture quickly
    if (m_showIDMap == true) {
        m_pDebugTexture->Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_idTexSplat);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glUniform1i(glGetUniformLocation(m_pDebugTexture->m_shaderID, "idTex"), 0);

        glDisable(GL_BLEND);           
        glBindVertexArray(m_quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    if (m_showNormals) {
        // draw white points
        m_pShaderPointsOnly->Use();
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "view"), 1, GL_FALSE,
            glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "proj"), 1, GL_FALSE,
            glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "model"), 1, GL_FALSE,
            glm::value_ptr(model));
        glUniform1f(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "pointSize"), splatSize);
        glBindVertexArray(m_lineVAO);
        glDrawArrays(GL_POINTS, 0, m_pointsAmount);

        // draw normal lines
        m_pShaderPointsNormals->Use();
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsNormals->m_shaderID, "view"), 1,
            GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsNormals->m_shaderID, "proj"), 1,
            GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsNormals->m_shaderID, "model"), 1,
            GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(m_lineVAO);
        glDrawArrays(GL_POINTS, 0, m_pointsAmount);

        if (m_showFrustum == true) {
            // Show Viewing Frustum
            
            m_pDrawFrustum->Use();

            glUniformMatrix4fv(glGetUniformLocation(m_pDrawFrustum->m_shaderID, "view"), 1, GL_FALSE,
                glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(m_pDrawFrustum->m_shaderID, "proj"), 1, GL_FALSE,
                glm::value_ptr(projection));
            glBindVertexArray(m_frustumVAO);
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        }
    }
    else {
        m_pShaderPointsOnly->Use();
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "view"), 1, GL_FALSE,
            glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "proj"), 1, GL_FALSE,
            glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "model"), 1, GL_FALSE,
            glm::value_ptr(model));
        glUniform1f(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "pointSize"), splatSize);
        glBindVertexArray(m_lineVAO);
        glDrawArrays(GL_POINTS, 0, m_pointsAmount);
    }
    glBindVertexArray(0);

    //RenderText(fps, m_pointCloud);
}

// VAO for the normal lines
GLuint Renderer::SetupLineVAO() {
    glGenVertexArrays(1, &m_lineVAO);
    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point),
        (void*)offsetof(Point, m_position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, m_normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, m_color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    return m_lineVAO;
}

// VAO for screen quad
GLuint Renderer::SetupQuadVAO() {
    float quadVertices[] = {
        -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f };

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

GLuint Renderer::SetupFrustumVAO(const glm::mat4& projection, const glm::mat4& view) {
    std::vector<glm::vec4> ndcCorners = {
        {-1, -1, -1, 1}, {1, -1, -1, 1}, {1, 1, -1, 1}, {-1, 1, -1, 1},  // Near plane
        {-1, -1, 1, 1},  {1, -1, 1, 1},  {1, 1, 1, 1},  {-1, 1, 1, 1}    // Far plane
    };

    glm::mat4 invViewProj = glm::inverse(projection * view);
    std::vector<glm::vec4> worldCorners;

    for (const auto& ndc : ndcCorners) {
        glm::vec4 world = invViewProj * ndc;
        worldCorners.push_back(world / world.w);
    }

    // Indices for 12 frustum indices
    std::vector<GLuint> indices = { 0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
                                   6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };

    GLuint frustumVBO, frustumEBO;
    glGenVertexArrays(1, &m_frustumVAO);
    glGenBuffers(1, &frustumVBO);
    glGenBuffers(1, &frustumEBO);

    glBindVertexArray(m_frustumVAO);

    glBindBuffer(GL_ARRAY_BUFFER, frustumVBO);
    glBufferData(GL_ARRAY_BUFFER, worldCorners.size() * sizeof(glm::vec4), worldCorners.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, frustumEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(),
        GL_STATIC_DRAW);

    glBindVertexArray(0);

    return m_frustumVAO;
}

/* -------------------------------------------------------------------------
 * Helper functions to read data from textures
 *
 * Reading the data from the generated normal texture and storing its content in
 * a vector. Also reading the ids from the helper ID texture and storing it in
 * an additional array.
 *
 * -------------------------------------------------------------------------
 */

void Renderer::ConfigureNormalSSBO() {

    glGenBuffers(1, &m_pointNormalSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointNormalSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Point) * m_pointsAmount, m_pointCloud.m_points.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_pointNormalSSBO);

}

void Renderer::ConfigureAvgSSBO() {
    glGenBuffers(1, &m_pointAvgSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointAvgSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Point) * m_pointsAmount,
m_pointCloud.m_points.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_pointAvgSSBO); 
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

/* -------------------------------------------------------------------------
 * configureFBO
 *
 * Creating three textures (Depth, ID, Normal) to render to with custom FBO
 * -------------------------------------------------------------------------
 */

void Renderer::ConfigureRefFBO() {
    glGenFramebuffers(1, &m_fboRef);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboRef);

    // depth tex
    glGenTextures(1, &m_depthTexRef);
    glBindTexture(GL_TEXTURE_2D, m_depthTexRef);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, m_width, m_height, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexRef, 0);


    // ID texture, for storing IDs for each Point of the Pointcloud
    glGenTextures(1, &m_idTexRef);
    glBindTexture(GL_TEXTURE_2D, m_idTexRef);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, m_width, m_height, 0, GL_RED_INTEGER, GL_INT,
        nullptr);

    const GLint minusOne[1] = { -1 };
    glClearTexImage(m_idTexRef, 0, GL_RED_INTEGER, GL_INT, minusOne);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_idTexRef, 0);

    GLenum attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        printf("Ref FBO complete!\n");
    }
    else {
        printf("FBO incomplete! Error: %d\n", status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ConfigureSplatFBO() {
    glGenFramebuffers(1, &m_fboSplat);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboSplat);

    // depth tex
    glGenTextures(1, &m_depthTexSplat);
    glBindTexture(GL_TEXTURE_2D, m_depthTexSplat);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, m_width, m_height, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexSplat, 0);


    // ID texture, for storing IDs for each Point of the Pointcloud
    glGenTextures(1, &m_idTexSplat);
    glBindTexture(GL_TEXTURE_2D, m_idTexSplat);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, m_width, m_height, 0, GL_RED_INTEGER, GL_INT,
        nullptr);

    const GLint minusOne[1] = { -1 };
    glClearTexImage(m_idTexSplat, 0, GL_RED_INTEGER, GL_INT, minusOne);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_idTexSplat, 0);

    GLenum attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        printf("Splat FBO complete!\n");
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


void Renderer::RenderText(float fps, PointCloud pc ) {
    glUseProgram(0);

    // Set up orthographic projection for 2D screen-space rendering (e.g., text)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, m_width, m_height, 0, -1, 1);

    std::stringstream ss;

    ss << "FPS: " << fps
        << "\nPoints: " << m_pointsAmount
        << "\nSplat Size: " << splatSize
        << "\nNormal (Point 222): " << glm::to_string(pc.GetNormalByID(222));
    std::string text = ss.str();

    static char buffer[99999];
    int num_quads = stb_easy_font_print(20, 20, (char*)text.c_str(), NULL, buffer, sizeof(buffer));

    glColor3f(0.0f, 1.0f, 0.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();
}

