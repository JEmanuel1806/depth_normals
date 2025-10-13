#include "App.h"


// ---------- Debug Output ------------- //
void GLAPIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* userParam)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }

    std::cerr << "[OpenGL DEBUG] " << message << std::endl;

    if (severity == GL_DEBUG_SEVERITY_HIGH)
        std::cerr << "Severity: HIGH\n";
    else if (severity == GL_DEBUG_SEVERITY_MEDIUM)
        std::cerr << "Severity: MEDIUM\n";
    else if (severity == GL_DEBUG_SEVERITY_LOW)
        std::cerr << "Severity: LOW\n";
}


App::App(unsigned int w, unsigned int h, std::string plyFile) : width(w), height(h) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(-1);
    }

    window = glfwCreateWindow(width, height, "Depth Buffer Normal Calculation", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        exit(-1);
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugCallback, nullptr);

    std::cout << "Loaded point cloud:" << plyFile;

    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double xpos, double ypos) {
        static_cast<App*>(glfwGetWindowUserPointer(win))->mouse_callback(win, xpos, ypos);
        });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int mods) {
        static_cast<App*>(glfwGetWindowUserPointer(win))
            ->mouse_button_callback(win, button, action, mods);
        });

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.Alpha = 0.8f;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // set camera and start renderer
    camera = new Camera(glm::vec3(0.0f, 0.0f, 6.0f));
    renderer = new Renderer(camera);
    renderer->Start(plyFile, width, height);

}

App::~App() {
    delete renderer;
    delete camera;
    glfwDestroyWindow(window);
    glfwTerminate();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void App::run() {

    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        // calculate FPS
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float fps = 1.0f / deltaTime;

        setupGUI(fps);
        processInput();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);

        // actual render pipeline start
        renderer->Render(fps);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void App::setupGUI(float fps)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::Begin("DockSpace Demo", nullptr, window_flags);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();

    ImGui::Begin("Control Panel");
    ImGui::Checkbox("Show Normals", &renderer->m_showNormals);
    ImGui::Checkbox("Show AABB", &renderer->m_showAABB);
    ImGui::Checkbox("Show ID Points", &renderer->m_showIDMap);
    if (ImGui::Button("Show Point Cloud")) {
        renderer->m_displayMode = Renderer::DisplayMode::POINTCLOUD;
    }
    if (ImGui::Button("Ground Truth (IPSR)")) {
        renderer->m_displayMode = Renderer::DisplayMode::IPSR_MESH;
    }
    if (ImGui::Button("Reconstruction (PSR)")) {
        renderer->m_displayMode = Renderer::DisplayMode::POISSON_MESH;
    }
    if (ImGui::Button("Show Occluded Normals")) {
        renderer->m_displayMode = Renderer::DisplayMode::POISSON_MESH;
    }
    if (ImGui::Button("Camera Angle")) {
        renderer->cameraViewPos = (renderer->cameraViewPos + 1) % 16;
    }
    if (ImGui::Button("Automatic Mode")) {
        if (renderer->automatic_mode)
            renderer->automatic_mode = false;
        else
            renderer->automatic_mode = true;
    }
    ImGui::Spacing();
    ImGui::Spacing();
    if (ImGui::Button("Save PLY File")) {
        renderer->saveToPLY = true;
    }
    if (ImGui::Button("Recalculate normals")) {
        renderer->m_recalculate = true;
    }
    ImGui::End();

    ImGui::Begin("Statistics");
    ImGui::Text("FPS: %.1f", fps);
    ImGui::InputFloat("Splat Size", &renderer->splatSize);
    ImGui::InputFloat("Depth Threshold", &renderer->depthThreshold);
    ImGui::InputInt("Normal", &renderer->normalDebugID);
    ImGui::Text("Point Cloud Size: %d", renderer->m_pointsAmount);
    uint32_t total = renderer->m_stats.occludedNrml + renderer->m_stats.goodNrml + renderer->m_stats.mediumNrml + renderer->m_stats.badNrml;
    ImGui::Text("Good:     %u", renderer->m_stats.goodNrml);
    ImGui::Text("Medium:   %u", renderer->m_stats.mediumNrml);
    ImGui::Text("Bad:      %u", renderer->m_stats.badNrml);
    ImGui::Text("Skipped/Occluded: %u", renderer->m_stats.occludedNrml);
    ImGui::Text("Total points with normals: %u", total - renderer->m_stats.occludedNrml);
    if (total) {
        ImGui::Text("Good %%:  %.1f%%", 100.f * float(renderer->m_stats.goodNrml) / float(total));
    }
    ImGui::Spacing();
    ImGui::Text("Goal to beat: 80%");
    ImGui::End();
}


// ---------- Control Handling (Keyboard & Mouse) ------------- //
void App::processInput() {
    auto isPressed = [&](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };

    if (isPressed(GLFW_KEY_W)) camera->ProcessKeyboard(FORWARD, deltaTime);
    if (isPressed(GLFW_KEY_S)) camera->ProcessKeyboard(BACKWARD, deltaTime);
    if (isPressed(GLFW_KEY_A)) camera->ProcessKeyboard(LEFT, deltaTime);
    if (isPressed(GLFW_KEY_D)) camera->ProcessKeyboard(RIGHT, deltaTime);
    if (isPressed(GLFW_KEY_Q)) camera->ProcessKeyboard(ROTATE_LEFT, deltaTime);
    if (isPressed(GLFW_KEY_E)) camera->ProcessKeyboard(ROTATE_RIGHT, deltaTime);

    auto toggle = [&](int key, bool& flag) {
        if (isPressed(key) && !key_pressed) {
            flag = !flag;
            key_pressed = true;
        }
        if (glfwGetKey(window, key) == GLFW_RELEASE) {
            key_pressed = false;
        }
        };

    // adjust point size of pointcloud 
    if (isPressed(GLFW_KEY_KP_ADD) && !key_pressed) {
        renderer->splatSize++;
        key_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_RELEASE) key_pressed = false;

    if (isPressed(GLFW_KEY_KP_SUBTRACT) && !key_pressed) {
        renderer->splatSize--;
        key_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_RELEASE) key_pressed = false;

    // manual recalculation of normals
    toggle(GLFW_KEY_TAB, renderer->m_recalculate = false);

    // rotation
    bool left = isPressed(GLFW_KEY_LEFT);
    bool right = isPressed(GLFW_KEY_RIGHT);
    renderer->m_spinPointCloudLeft = left;
    renderer->m_spinPointCloudRight = right;
}

void App::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        left_mouse_pressed = (action == GLFW_PRESS);
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
        right_mouse_pressed = (action == GLFW_PRESS);
}

void App::mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    if (left_mouse_pressed && (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)) {
        renderer->lightYaw += xoffset * -0.1f;
        renderer->lightPitch += yoffset * 0.1f;

        if (renderer->lightPitch > 89.0f)  renderer->lightPitch = 89.0f;
        if (renderer->lightPitch < -89.0f) renderer->lightPitch = -89.0f;

        float radius = 10.0f;
        renderer->lightPos.x = radius * cos(glm::radians(renderer->lightYaw)) * cos(glm::radians(renderer->lightPitch));
        renderer->lightPos.y = radius * sin(glm::radians(renderer->lightPitch));
        renderer->lightPos.z = radius * sin(glm::radians(renderer->lightYaw)) * cos(glm::radians(renderer->lightPitch));
    }
    else if (left_mouse_pressed)
        camera->ProcessMouseMovement(xoffset, yoffset);
    else if (right_mouse_pressed)
        camera->ProcessMousePan(xoffset, yoffset);
}


