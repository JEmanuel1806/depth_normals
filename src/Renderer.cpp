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

#include <thread>
#include <chrono>

#include "Renderer.h"
#include "glm/gtx/string_cast.hpp"

Renderer::Renderer(Camera* cam) {
	m_pCamera = cam;
	m_pShaderDepth = nullptr;
	m_pShaderBigSplats = nullptr;
	m_pShaderPointsOnly = nullptr;
	m_pShaderMesh = nullptr;
	m_pShaderCalcNormal = nullptr;
	m_pShaderNormalAvg = nullptr;
	m_pShaderNormalCompute = nullptr;
	m_pShaderEvaluateNormal = nullptr;
	m_pShaderPointsNormals = nullptr;
	m_pDebugTexture = nullptr;
	m_VAO = 0;
	m_VBO = 0;
	m_lineVAO = 0;
	m_quadVAO = 0;
	m_AABO_VAO = 0;
}

Renderer::~Renderer() {
	delete m_pShaderDepth;
	delete m_pShaderBigSplats;
	delete m_pShaderPointsOnly;
	delete m_pShaderMesh;
	delete m_pShaderCalcNormal;
	delete m_pShaderNormalAvg;
	delete m_pShaderPointsNormals;
	delete m_pShaderNormalCompute;
	delete m_pShaderEvaluateNormal;
	delete m_pDebugTexture;
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
	glDeleteVertexArrays(1, &m_lineVAO);
	glDeleteVertexArrays(1, &m_quadVAO);
	glDeleteVertexArrays(1, &m_AABO_VAO);
}

// ---------- Initializes shaders, loads point cloud data, uploads it to the GPU, configures vertex attributes and framebuffers ------------- //
void Renderer::Start(std::string ply_path, unsigned int width, unsigned int height) {

	m_pShaderDepth = new Shader("src/shaders/depth_pass.vert", "src/shaders/depth_pass.frag");
	m_pShaderBigSplats = new Shader("src/shaders/biggerSplat_pass.vert", "src/shaders/biggerSplat_pass.frag");
	m_pShaderPointsOnly = new Shader("src/shaders/draw_points.vert", "src/shaders/draw_points.frag");
	m_pShaderMesh = new Shader("src/shaders/draw_mesh.vert", "src/shaders/draw_mesh.frag");
	m_pShaderCalcNormal = new Shader("src/shaders/calc_normal.vert", "src/shaders/calc_normal.frag");
	m_pShaderNormalCompute = new Shader("src/shaders/calc_normal.comp");
	m_pShaderNormalAvg = new Shader("src/shaders/average_normal.comp");
	m_pShaderEvaluateNormal = new Shader("src/shaders/evaluate_normals.comp");
	m_pShaderPointsNormals = new Shader("src/shaders/draw_lines.vert", "src/shaders/draw_lines.geom",
		"src/shaders/draw_lines.frag");
	m_pDebugTexture =
		new Shader("src/shaders/debug/debug_id_tex.vert", "src/shaders/debug/debug_id_tex.frag");
	m_pDrawAABB = new Shader("src/shaders/draw_AABB.vert", "src/shaders/draw_AABB.frag");

	m_width = width;
	m_height = height;

	// for time measurment
	glGenQueries(1, &qRef);
	glGenQueries(1, &qSplat);
	glGenQueries(1, &qAcc);
	glGenQueries(1, &qFin);
	glGenQueries(1, &qReadBack);
	glGenQueries(1, &t0);
	glGenQueries(1, &t1);

	// take ply_path and replace path with "ground truth" to get reference model from Ground Truth folder
	std::string ply_path_reference = ply_path;
	std::string term = "no_normals";

	size_t pos = ply_path_reference.find("no_normals");

	if (pos != std::string::npos) {
		ply_path_reference.replace(pos, term.length(), "ground_truth");
	}

	// Load point cloud from PLY file
	m_pointCloud = plyLoader.LoadPLY(ply_path); // no normal model, to be calculated
	m_pointCloudGT = plyLoader.LoadPLY(ply_path_reference); // ground truth
	m_pointsAmount = m_pointCloud.PointsAmount();
	m_pointsAmountGT = m_pointCloudGT.PointsAmount();

	if (m_pointsAmount != m_pointsAmountGT) {
		std::cerr << "Warning. Point cloud sizes dont match! \n";
	}

	m_VAO = SetupCloudVAO();

	std::cout << "Rendering " << m_pointsAmount << " points.\n";
	std::cout << "sizeof(Point): " << sizeof(Point) << std::endl;

	
	m_lineVAO = SetupLineVAO();
	m_quadVAO = SetupQuadVAO();
	ConfigureRefFBO();
	ConfigureSplatFBO();
	ConfigureAvgSSBO();
	ConfigureNormalSSBO();
	ConfigureGTSSBO();
	ConfigureStatsSSBO();
	ConfigureDensitySSBO();

	aabb = CalcAABB(m_pointCloud); // Bounding Box of Point Cloud
	SetupBBoxVAO(aabb);

	GLint currentFB;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFB);
	std::cout << "Current framebuffer: " << currentFB << std::endl;

	if (m_pointCloud.m_hasNormals) {
		std::cout << "Normals detected." << std::endl;
		std::cout << "Expected Normal for ID: " << 200 << " : " << glm::to_string(expectedNormal)
			<< std::endl;
	}
	else {
		std::cout << "No normals detected." << std::endl;
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
   
	glm::vec3 viewPos = m_pCamera->m_vecPosition;
	glm::vec3 lightColor = glm::vec3(1.0f);            
	glm::vec3 objectColor = glm::vec3(0.0f, 0.7f, 1.0f); 

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

	expectedNormal = m_pointCloud.GetNormalByID(200);

	std::vector<float> cameraAngles = {
	22.5f, 45, 67.5f,
	90, 112.5f, 135, 157.5f,
	180, 202.5f, 225, 247.5f,
	270, 292.5f, 315, 337.5f, 0.0f
	};

	glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0, 1.0, 0.0));

	glClearColor(0.141f, 0.149f, 0.192f, 1.0f);

	// position the frame capture camera in relation to the Bounding Box of the point cloud -> guarnatee consistent view

	float fovY = glm::radians(m_pCamera->m_zoom);
	float aspect = float(m_width) / float(m_height);
	float fovX = 2.0f * atan(tan(fovY * 0.5f) * aspect);

	float d_y = aabb.extent().y / tan(fovY * 0.5f);
	float d_x = aabb.extent().x / tan(fovX * 0.5f);

	float radius = glm::length(aabb.extent());
	float distance = radius / sin(fovY * 0.5f);
	distance *= 1.2f;

	glm::vec3 baseCamPos = aabb.center() + glm::vec3(0, 0, distance);
	glm::mat4 baseView = glm::lookAt(baseCamPos, aabb.center(), glm::vec3(0, 1, 0));


	if (!m_pointCloud.m_hasNormals && m_recalculate) {
		// automatic mode, predefined views for normal estimation
		if (automatic_mode) {

			int nHoriz = (int)cameraAngles.size();

			for (int i = 0; i < nHoriz + 4; ++i) {
				glm::mat4 view = baseView;
				glm::vec3 camPos;
	
				if (i < nHoriz) {
					float angle = glm::radians(cameraAngles[i]);
					glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));
					glm::vec3 offset = rot * glm::vec4(0, 0, distance, 1.0);
					camPos = aabb.center() + offset;
					view = glm::lookAt(camPos, aabb.center(), glm::vec3(0, 1, 0));

				}
				else if (i == nHoriz) {
					camPos = aabb.center() + glm::vec3(0, distance, 0);
					view = glm::lookAt(camPos, aabb.center(), glm::vec3(0, 0, -1));
				}
				else if (i == nHoriz + 1) {
					camPos = aabb.center() + glm::vec3(0, -distance, 0);
					view = glm::lookAt(camPos, aabb.center(), glm::vec3(0, 0, 1));
				}
				else if (i == nHoriz + 2) {
					float angle = glm::radians(45.0f);
					glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1, 0, 0));
					glm::vec3 offset = rot * glm::vec4(0, 0, distance, 1.0);
					camPos = aabb.center() + offset;
					view = glm::lookAt(camPos, aabb.center(), glm::vec3(0, 1, 0));
				}
				else if (i == nHoriz + 3) {
					float angle = glm::radians(-45.0f);
					glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1, 0, 0));
					glm::vec3 offset = rot * glm::vec4(0, 0, distance, 1.0);
					camPos = aabb.center() + offset;
					view = glm::lookAt(camPos, aabb.center(), glm::vec3(0, 1, 0));
				}
				else if (i == nHoriz + 4) {
					float angle = glm::radians(45.0f);
					glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1));
					glm::vec3 offset = rot * glm::vec4(distance, 0, 0, 1.0);
					camPos = aabb.center() + offset;
					view = glm::lookAt(camPos, aabb.center(), glm::vec3(0, 1, 0));
				}
				else if (i == nHoriz + 5) {
					float angle = glm::radians(-45.0f);
					glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1));
					glm::vec3 offset = rot * glm::vec4(distance, 0, 0, 1.0);
					camPos = aabb.center() + offset;
					view = glm::lookAt(camPos, aabb.center(), glm::vec3(0, 1, 0));
				}
				else if (i == nHoriz + 6) {
					glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1, 0, 0));
					rot = glm::rotate(rot, glm::radians(45.0f), glm::vec3(0, 1, 0));
					glm::vec3 offset = rot * glm::vec4(0, 0, distance, 1.0);
					camPos = aabb.center() + offset;
					view = glm::lookAt(camPos, aabb.center(), glm::vec3(0, 1, 0));
				}
				else if (i == nHoriz + 7) {
					glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(1, 0, 0));
					rot = glm::rotate(rot, glm::radians(45.0f), glm::vec3(0, 1, 0));
					glm::vec3 offset = rot * glm::vec4(0, 0, distance, 1.0);
					camPos = aabb.center() + offset;
					view = glm::lookAt(camPos, aabb.center(), glm::vec3(0, 1, 0));
				}
				
				ComputeNormalsForView(view, projection, model);
			}
		}
		// manual mode, normals update with camera view
		else {
			ComputeNormalsForView(view, projection, model);
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_POINT_SMOOTH);


	if (m_showAABB == true) {

		m_pDrawAABB->Use();

		glUniformMatrix4fv(glGetUniformLocation(m_pDrawAABB->m_shaderID, "view"), 1, GL_FALSE,
			glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(m_pDrawAABB->m_shaderID, "proj"), 1, GL_FALSE,
			glm::value_ptr(projection));
		glBindVertexArray(m_AABO_VAO);
		glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
	}

	// for debugging any texture quickly
	if (m_showIDMap) {
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

	else if (m_showNormals) {
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
	}
	else if (m_displayMode == DisplayMode::POINTCLOUD) {
		m_pShaderPointsOnly->Use();
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "view"), 1, GL_FALSE,
			glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "proj"), 1, GL_FALSE,
			glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(m_pShaderPointsOnly->m_shaderID, "model"), 1, GL_FALSE,
			glm::value_ptr(model));

		glBindVertexArray(m_lineVAO);
		glDrawArrays(GL_POINTS, 0, m_pointsAmount);
	}
	else if (m_displayMode == DisplayMode::IPSR_MESH) {
		if (m_meshVAO_IPSR) {
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			m_pShaderMesh->Use();
			glUniformMatrix4fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "view"), 1, GL_FALSE,
				glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "proj"), 1, GL_FALSE,
				glm::value_ptr(projection));
			glUniformMatrix4fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "model"), 1, GL_FALSE,
				glm::value_ptr(model));

			glUniform3fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "lightPos"), 1, glm::value_ptr(lightPos));
			glUniform3fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "viewPos"), 1, glm::value_ptr(viewPos));
			glUniform3fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "lightColor"), 1, glm::value_ptr(lightColor));
			glUniform3fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "objectColor"), 1, glm::value_ptr(objectColor));

			glBindVertexArray(m_meshVAO_IPSR);
			glDrawElements(GL_TRIANGLES, m_meshIndexCount_IPSR, GL_UNSIGNED_INT, 0);
		}
	}
	else if (m_displayMode == DisplayMode::POISSON_MESH) {
		if (m_meshVAO_Poisson) {
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			m_pShaderMesh->Use();
			glUniformMatrix4fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "view"), 1, GL_FALSE,
				glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "proj"), 1, GL_FALSE,
				glm::value_ptr(projection));
			glUniformMatrix4fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "model"), 1, GL_FALSE,
				glm::value_ptr(model));

			glUniform3fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "lightPos"), 1, glm::value_ptr(lightPos));
			glUniform3fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "viewPos"), 1, glm::value_ptr(viewPos));
			glUniform3fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "lightColor"), 1, glm::value_ptr(lightColor));
			glUniform3fv(glGetUniformLocation(m_pShaderMesh->m_shaderID, "objectColor"), 1, glm::value_ptr(objectColor));

			glBindVertexArray(m_meshVAO_Poisson);
			glDrawElements(GL_TRIANGLES, m_meshIndexCount_Poisson, GL_UNSIGNED_INT, 0);
		}
	}
	glBindVertexArray(0);

	// saving the new point cloud to a file
	// calling ipsr on the point cloud without normals
	// calling screened poisson on the new point cloud 
	if (saveToPLY) {
		std::string inputPath = "data/custom/output_data/output.ply";
		std::string outputPath = "data/custom/output_data/output_recon.ply";
		std::string outputPathIPSR = "data/custom/output_data/output_ipsr.ply";

		plyLoader.SavePLY(inputPath, m_pointCloud);
		std::cout << "Exported ply file! \n";

		//CommandLine ipsr("ipsr/ipsr.exe");
		//ipsr.arg("--in");
		//ipsr.arg("data/ipsr_data/bimba.ply");
		//ipsr.arg("--out");
		//ipsr.arg(outputPathIPSR);
		//
		//int exitCode = ipsr.executeAndWait();

		CommandLine poisson("poisson/PoissonRecon.exe");
		poisson.arg("--in");
		poisson.arg(inputPath);
		poisson.arg("--out");
		poisson.arg(outputPath);
		poisson.arg("--depth");
		poisson.arg("10");
		poisson.arg("--samplesPerNode");
		poisson.arg("1.5");
		poisson.arg("--pointWeight");
		poisson.arg("10");
		poisson.arg("--ascii");


		int exitCode2 = poisson.executeAndWait();
		std::cout << "PoissonRecon finished with code " << exitCode2 << std::endl;

		m_meshIPSR = plyLoader.LoadPLY(outputPathIPSR);
		m_meshPoisson = plyLoader.LoadPLY(outputPath);

		if (!m_meshIPSR.m_faces.empty()) {
			ComputeMeshNormals(m_meshIPSR);
			m_meshVAO_IPSR = SetupMeshVAO(m_meshIPSR);
			m_meshIndexCount_IPSR = 0;
			std::cout << "setup IPSR mesh VAO\n";
			for (auto& f : m_meshIPSR.m_faces)
				m_meshIndexCount_IPSR += (GLuint)f.indices.size();
		}

		if (!m_meshPoisson.m_faces.empty()) {
			ComputeMeshNormals(m_meshPoisson);
			m_meshVAO_Poisson = SetupMeshVAO(m_meshPoisson);
			m_meshIndexCount_Poisson = 0;
			std::cout << "setup Poisson mesh VAO\n";
			for (auto& f : m_meshPoisson.m_faces)
				m_meshIndexCount_Poisson += (GLuint)f.indices.size();
		}

		saveToPLY = false;
	}

	RenderText(fps, m_pointCloud, m_pointCloudGT, normalDebugID);
}


void Renderer::ComputeNormalsForView(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& model) {

	// ---------- FIRST PASS ------------- //
	glBeginQuery(GL_TIME_ELAPSED, qRef);
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
	glEndQuery(GL_TIME_ELAPSED);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// ---------- SECOND PASS ------------- //
	glBeginQuery(GL_TIME_ELAPSED, qSplat);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fboSplat);
	//glDepthMask(GL_FALSE);
	//glDisable(GL_BLEND);

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
	glEndQuery(GL_TIME_ELAPSED);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);


	// ---------- THIRD PASS ------------- //
	glBeginQuery(GL_TIME_ELAPSED, qAcc);
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
	glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "invView"), 1, GL_FALSE,
		glm::value_ptr(glm::inverse(view)));
	glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "proj"), 1, GL_FALSE,
		glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "invProj"), 1,
		GL_FALSE, glm::value_ptr(glm::inverse(projection)));
	glUniform1f(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "zNear"), m_zNear);
	glUniform1f(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "zFar"), m_zFar);
	glUniform1f(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "maxID"), m_pointsAmount);
	glUniform1f(glGetUniformLocation(m_pShaderNormalCompute->m_shaderID, "depthThreshold"), depthThreshold);


	// compute shader vars

	GLuint workGroupX = (m_width + 7) / 8;
	GLuint workGroupY = (m_height + 7) / 8;
	glClearNamedBufferData(m_pointNormalSSBO, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
	glClearNamedBufferData(m_densitySSBO, GL_RG32F, GL_RG, GL_FLOAT, nullptr);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_pointNormalSSBO);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_densitySSBO);

	glDispatchCompute(workGroupX, workGroupY, 1);

	glDispatchCompute(workGroupX, workGroupY, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


	// Adaptive Splat Size Readback
	std::vector<DensityBuffer> densities(m_pointsAmount);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_densitySSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
		sizeof(DensityBuffer)* m_pointsAmount,
		densities.data());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	
	double avgDensity = 0.0;
	for (auto& d : densities) {
		avgDensity += (double)d.counter;
	}
	avgDensity /= double(m_pointsAmount);
	
	splatSize = glm::clamp((float)(1.0 + avgDensity * 2), 1.0f, 1000.0f);

	glEndQuery(GL_TIME_ELAPSED);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// ---------- FOURTH PASS ------------- //
	glBeginQuery(GL_TIME_ELAPSED, qFin);
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
	glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "invView"), 1, GL_FALSE,
		glm::value_ptr(glm::inverse(view)));
	glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "proj"), 1, GL_FALSE,
		glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "invProj"), 1,
		GL_FALSE, glm::value_ptr(glm::inverse(projection)));
	glUniform1f(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "zNear"), m_zNear);
	glUniform1f(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "zFar"), m_zFar);
	glUniform1i(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "maxID"), m_pointsAmount);

	glUniform1f(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "goodNormal"), goodNormal);
	glUniform1f(glGetUniformLocation(m_pShaderNormalAvg->m_shaderID, "badNormal"), badNormal);


	// compute shader vars
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_pointNormalSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_pointGTSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_pointAvgSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_statsSSBO);

	glDispatchCompute(workGroupX, workGroupY, 1);

	glEndQuery(GL_TIME_ELAPSED);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	glBeginQuery(GL_TIME_ELAPSED, qReadBack);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointAvgSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Point) * m_pointsAmount, m_pointCloud.m_points.data());

	// back to VBO for arrow vis
	glBindBuffer(GL_COPY_READ_BUFFER, m_pointAvgSSBO);
	glBindBuffer(GL_COPY_WRITE_BUFFER, m_VBO);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(Point) * m_pointsAmount);

	glEndQuery(GL_TIME_ELAPSED);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);	

	glQueryCounter(t1, GL_TIMESTAMP);


	GLuint64 nsRef = 0, nsSplat = 0, nsAcc = 0, nsAvg = 0, nsRB = 0, ts0 = 0, ts1 = 0;
	glGetQueryObjectui64v(qRef, GL_QUERY_RESULT, &nsRef);
	glGetQueryObjectui64v(qSplat, GL_QUERY_RESULT, &nsSplat);
	glGetQueryObjectui64v(qAcc, GL_QUERY_RESULT, &nsAcc);
	glGetQueryObjectui64v(qFin, GL_QUERY_RESULT, &nsAvg);
	glGetQueryObjectui64v(qReadBack, GL_QUERY_RESULT, &nsRB);
	glGetQueryObjectui64v(t0, GL_QUERY_RESULT, &ts0);
	glGetQueryObjectui64v(t1, GL_QUERY_RESULT, &ts1);

	double msRef = nsRef / 1e6;
	double msSplat = nsSplat / 1e6;
	double msAcc = nsAcc / 1e6;
	double msAvg = nsAvg / 1e6;
	double msRB = nsRB / 1e6;
	double msTotal = msRef + msSplat + msAcc + msAvg;


	std::cout << "Depth Tex  : " << msRef << " ms\n"
		<< "Generate Splats: " << msSplat << " ms\n"
		<< "Accumulate Normals  : " << msAcc << " ms\n"
		<< "Final Averaging  : " << msAvg << " ms\n"
		<< "Normal Calc (Acc + Final): " << msAvg + msAcc << " ms\n"
		<< "Total (no Readback): " << msTotal << " ms  ->  " << 1000 / (msTotal) << " FPS\n"
		<< "Readback to VBO for vis: " << msRB << " ms\n";
	
	ComputeNormalStatsGPU(goodNormal, badNormal);
}

// Compute normal qualitiy statistics
void Renderer::ComputeNormalStatsGPU(float goodDeg, float badDeg) {
	NormalStats zero = { 0,0,0,0 };
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_statsSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(NormalStats), &zero);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glUseProgram(m_pShaderEvaluateNormal->m_shaderID);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_pointGTSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_pointAvgSSBO); 
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_statsSSBO);

	glUniform1ui(glGetUniformLocation(m_pShaderEvaluateNormal->m_shaderID, "uCount"), (GLuint)m_pointsAmount);
	glUniform1f(glGetUniformLocation(m_pShaderEvaluateNormal->m_shaderID, "uGoodDeg"), goodDeg);
	glUniform1f(glGetUniformLocation(m_pShaderEvaluateNormal->m_shaderID, "uBadDeg"), badDeg);

	const GLuint wg = 256;
	GLuint groups = (GLuint)((m_pointsAmount + wg - 1) / wg);
	glDispatchCompute(groups, 1, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_statsSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(NormalStats), &m_stats);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

// Compute normals for reconstructed mesh to enable correct shading
void Renderer::ComputeMeshNormals(PointCloud& mesh) {

	for (auto& v : mesh.m_points) {
		v.m_normal = glm::vec3(0.0f);
	}


	for (auto& f : mesh.m_faces) {
		glm::vec3 A = mesh.m_points[f.indices[0]].m_position;
		glm::vec3 B = mesh.m_points[f.indices[1]].m_position;
		glm::vec3 C = mesh.m_points[f.indices[2]].m_position;

		glm::vec3 n = glm::normalize(glm::cross(B - A, C - A));


		mesh.m_points[f.indices[0]].m_normal += n;
		mesh.m_points[f.indices[1]].m_normal += n;
		mesh.m_points[f.indices[2]].m_normal += n;
	}


	for (auto& v : mesh.m_points) {
		v.m_normal = glm::normalize(v.m_normal);
	}

	mesh.m_hasNormals = true; 
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

GLuint Renderer::SetupCloudVAO()
{
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

	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Point),
		(void*)offsetof(Point, m_splatSize));
	glEnableVertexAttribArray(3);

	return m_VAO;
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

GLuint Renderer::SetupMeshVAO(const PointCloud& pc) {
	GLuint vao, vbo, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, pc.m_points.size() * sizeof(Point), pc.m_points.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, m_position));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, m_normal));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, m_color));
	glEnableVertexAttribArray(2);

	std::vector<GLuint> indices;
	for (auto& f : pc.m_faces) {
		for (int idx : f.indices) {
			indices.push_back(idx);
		}
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
	return vao;
}


GLuint Renderer::SetupBBoxVAO(const BoundingBox& box)
{
	std::vector<glm::vec3> corners = {
	{box.min.x, box.min.y, box.min.z},
	{box.max.x, box.min.y, box.min.z},
	{box.max.x, box.max.y, box.min.z},
	{box.min.x, box.max.y, box.min.z},
	{box.min.x, box.min.y, box.max.z},
	{box.max.x, box.min.y, box.max.z},
	{box.max.x, box.max.y, box.max.z},
	{box.min.x, box.max.y, box.max.z}
	};

	std::vector<GLuint> indices = {
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		0, 4, 1, 5, 2, 6, 3, 7
	};

	GLuint bboxVBO, bboxEBO;
	glGenVertexArrays(1, &m_AABO_VAO);
	glGenBuffers(1, &bboxVBO);
	glGenBuffers(1, &bboxEBO);

	glBindVertexArray(m_AABO_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, bboxVBO);
	glBufferData(GL_ARRAY_BUFFER, corners.size() * sizeof(glm::vec3), corners.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bboxEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);

	return m_AABO_VAO;
}

void Renderer::ConfigureNormalSSBO() {

	glGenBuffers(1, &m_pointNormalSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointNormalSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Point) * m_pointsAmount, m_pointCloud.m_points.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_pointNormalSSBO);

}

void Renderer::ConfigureGTSSBO() {

	glGenBuffers(1, &m_pointGTSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointGTSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Point) * m_pointsAmountGT, m_pointCloudGT.m_points.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_pointGTSSBO);

}

void Renderer::ConfigureAvgSSBO() {
	glGenBuffers(1, &m_pointAvgSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointAvgSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Point) * m_pointsAmount,
		m_pointCloud.m_points.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_pointAvgSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::ConfigureStatsSSBO() {
	glGenBuffers(1, &m_statsSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_statsSSBO);

	NormalStats zeroStats = { 0, 0, 0, 0 }; 
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(NormalStats),&zeroStats,GL_DYNAMIC_DRAW);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_statsSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::ConfigureDensitySSBO() {

	glGenBuffers(1, &m_densitySSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_densitySSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		m_pointsAmount * sizeof(DensityBuffer),
		nullptr,
		GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_densitySSBO); // binding = 4 wie im Shader
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}

void Renderer::ConfigureRefFBO() {
	glGenFramebuffers(1, &m_fboRef);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fboRef);

	// depth tex
	glGenTextures(1, &m_depthTexRef);
	glBindTexture(GL_TEXTURE_2D, m_depthTexRef);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_width, m_height, 0,
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_width, m_height, 0,
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

Renderer::BoundingBox Renderer::CalcAABB(PointCloud& pc) {

	BoundingBox boundingBox;

	glm::vec3 bboxMin = glm::vec3(std::numeric_limits<float>::infinity());
	glm::vec3 bboxMax = glm::vec3(-std::numeric_limits<float>::infinity());


	for (int i = 0; i < pc.PointsAmount(); ++i) {
		auto point = pc.GetPointByID(i)->GetPosition();

		// MAX
		if (point.x > bboxMax.x) {
			bboxMax.x = point.x;
		}
		if (point.y > bboxMax.y) {
			bboxMax.y = point.y;
		}
		if (point.z > bboxMax.z) {
			bboxMax.z = point.z;
		}

		// MIN
		if (point.x < bboxMin.x) {
			bboxMin.x = point.x;
		}
		if (point.y < bboxMin.y) {
			bboxMin.y = point.y;
		}
		if (point.z < bboxMin.z) {
			bboxMin.z = point.z;
		}
	}

	boundingBox.min = bboxMin;
	boundingBox.max = bboxMax;

	return boundingBox;

}

void Renderer::RenderText(float fps, PointCloud pc, PointCloud pcGT, int id) {
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
		<< "\nNormal (Point " << id << "): " << glm::to_string(pc.GetNormalByID(id))
		<< "\nExpected (Point " << id << "): " << glm::to_string(pcGT.GetNormalByID(id));
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

