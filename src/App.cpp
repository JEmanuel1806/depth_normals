#include "App.h"


// Debug output for debugging (obv)
void GLAPIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* userParam) {
    std::cerr << "[OpenGL DEBUG] " << message << std::endl;

    if (severity == GL_DEBUG_SEVERITY_HIGH)
        std::cerr << "Severity: HIGH\n";
    else if (severity == GL_DEBUG_SEVERITY_MEDIUM)
        std::cerr << "Severity: MEDIUM\n";
    else if (severity == GL_DEBUG_SEVERITY_LOW)
        std::cerr << "Severity: LOW\n";
}


// ply point cloud input given
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

    // DEBUG OUTPUT
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    
    ImGui::StyleColorsDark();

    // Backend: GLFW + OpenGL3
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    camera = new Camera(glm::vec3(0.0f, 0.0f, 6.0f));
    renderer = new Renderer(camera);

    // calculation
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

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float fps = 1.0f / deltaTime;

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
        ImGui::Checkbox("Show Points", &renderer->m_showPoints);
        ImGui::End();


        ImGui::Begin("Statistics");
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("Splat Size: %d", renderer->splatSize);
        ImGui::End();

        processInput();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);
        renderer->Render(fps);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void App::processInput() {
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantCaptureKeyboard)
        return;

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

    // debugging the normals
    toggle(GLFW_KEY_N, renderer->m_showNormals);
    renderer->m_showNormals;

    // debugging the textures
    toggle(GLFW_KEY_I, renderer->m_showIDMap);
    renderer->m_showIDMap;

    if (isPressed(GLFW_KEY_S) && (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) && !key_pressed) {
        renderer->saveToPLY = true;   
        key_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) {
        key_pressed = false;
    }

    if (isPressed(GLFW_KEY_LEFT_ALT)) {
        renderer->m_showPoints = false;
        key_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE) {
        renderer->m_showPoints = true;
    }

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

    // visualize frustum cone
    toggle(GLFW_KEY_F, renderer->m_showAABB);
    renderer->m_showAABB;

    toggle(GLFW_KEY_TAB, renderer->m_recalculate = false);

    // rotation
    bool left = isPressed(GLFW_KEY_LEFT);
    bool right = isPressed(GLFW_KEY_RIGHT);
    renderer->m_spinPointCloudLeft = left;
    renderer->m_spinPointCloudRight = right;
}

void App::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return; // Maus gehört gerade ImGui

    if (button == GLFW_MOUSE_BUTTON_LEFT)
        left_mouse_pressed = (action == GLFW_PRESS);
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
        right_mouse_pressed = (action == GLFW_PRESS);
}

void App::mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return; 

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

    if (left_mouse_pressed)
        camera->ProcessMouseMovement(xoffset, yoffset);
    else if (right_mouse_pressed)
        camera->ProcessMousePan(xoffset, yoffset);
}


