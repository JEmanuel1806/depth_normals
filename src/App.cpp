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

App::App(unsigned int w, unsigned int h) : width(w), height(h) {
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

    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double xpos, double ypos) {
        static_cast<App*>(glfwGetWindowUserPointer(win))->mouse_callback(win, xpos, ypos);
        });
    glfwSetScrollCallback(window, [](GLFWwindow* win, double xoffset, double yoffset) {
        static_cast<App*>(glfwGetWindowUserPointer(win))->scroll_callback(win, xoffset, yoffset);
        });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int mods) {
        static_cast<App*>(glfwGetWindowUserPointer(win))
            ->mouse_button_callback(win, button, action, mods);
        });

    camera = new Camera(glm::vec3(0.0f, 0.0f, 4.0f));
    renderer_left = new Renderer(camera);
    renderer_right = new Renderer(camera);

    renderer_left->Start("data/custom/no_normals/plane_sparse_no_normals.ply", width/2, height);
    renderer_right->Start("data/custom/ground_truth/plane_sparse.ply", width/2, height);
}

App::~App() {
    delete renderer_left;
    delete renderer_right;
    delete camera;
    glfwDestroyWindow(window);
    glfwTerminate();
}

void App::run() {
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float fps = 1.0f / deltaTime;

        processInput();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
       
        int viewportWidth = width / 2;

        // left viewport
        glViewport(0, 0, viewportWidth, height);
        renderer_left->Render(fps);

        // right viewport
        glViewport(viewportWidth, 0, viewportWidth, height);
        renderer_right->Render(fps);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

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

    // debugging the normals
    toggle(GLFW_KEY_N, renderer_left->m_showNormals);
    renderer_right->m_showNormals = renderer_left->m_showNormals;

    // debugging the textures
    toggle(GLFW_KEY_I, renderer_left->m_showIDMap);
    renderer_right->m_showIDMap = renderer_left->m_showIDMap;

    toggle(GLFW_KEY_O, renderer_left->m_showNormalMap);
    renderer_right->m_showNormalMap = renderer_left->m_showNormalMap;

    // adjust point size of pointcloud 
    if (isPressed(GLFW_KEY_KP_ADD) && !key_pressed) {
        renderer_left->pointSize++;
        renderer_right->pointSize++;
        key_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_RELEASE) key_pressed = false;

    if (isPressed(GLFW_KEY_KP_SUBTRACT) && !key_pressed) {
        renderer_left->pointSize--;
        renderer_right->pointSize--;
        key_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_RELEASE) key_pressed = false;

    // visualize frustum cone
    toggle(GLFW_KEY_F, renderer_left->m_showFrustum);
    renderer_right->m_showFrustum = renderer_left->m_showFrustum;

    toggle(GLFW_KEY_TAB, renderer_left->m_recalculate = false);

    // rotation
    bool left = isPressed(GLFW_KEY_LEFT);
    bool right = isPressed(GLFW_KEY_RIGHT);
    renderer_left->m_spinPointCloudLeft = left;
    renderer_left->m_spinPointCloudRight = right;
    renderer_right->m_spinPointCloudLeft = left;
    renderer_right->m_spinPointCloudRight = right;
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

    if (left_mouse_pressed)
        camera->ProcessMouseMovement(xoffset, yoffset);
    else if (right_mouse_pressed)
        camera->ProcessMousePan(xoffset, yoffset);
}

void App::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera->ProcessMouseScroll(static_cast<float>(yoffset));
}
