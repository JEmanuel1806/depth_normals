#include "App.h"


App::App(unsigned int width, unsigned height)
{

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

	glfwMakeContextCurrent(window);
	glfwSetWindowUserPointer(window, this); 
	glfwSetCursorPosCallback(window, [](GLFWwindow* win, double xpos, double ypos) {
		App* app = static_cast<App*>(glfwGetWindowUserPointer(win));
		if (app) app->mouse_callback(win, xpos, ypos);
		});

	glfwSetScrollCallback(window, [](GLFWwindow* win, double xoffset, double yoffset) {
		App* app = static_cast<App*>(glfwGetWindowUserPointer(win));
		if (app) app->scroll_callback(win, xoffset, yoffset);
		});

	glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int mods) {
		App* app = static_cast<App*>(glfwGetWindowUserPointer(win));
		if (app) app->mouse_button_callback(win, button, action, mods);
		});

	camera = new Camera(glm::vec3(0.0f, 0.0f, 4.0f));
	
	renderer_left = new Renderer(camera);
	renderer_right = new Renderer(camera);
	renderer_left->Start("data/custom/ground_truth/plane.ply");
	renderer_right->Start("data/custom/no_normals/plane_no_normals.ply");

	run(width, height);
}

App::~App() {
	delete renderer_left;
	delete renderer_right;
	delete camera;
	glfwDestroyWindow(window);
	glfwTerminate();
}

void App::run(unsigned int width, unsigned int height) {

	while (!glfwWindowShouldClose(window)) {

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		float fps = 1.0f / deltaTime;

		processInput(window);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// left viewport for ground truth
		glViewport(0, 0, (float)width / 2, (float)height);
		renderer_left->Render((float)width / 2, (float)height, fps);

		// right viewport for calculated normal
		glViewport((float)width / 2, 0, (float)width / 2, (float)height);
		renderer_right->Render((float)width / 2, (float)height, fps);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void App::processInput(GLFWwindow* window)
{
	// movement
	
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera->ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera->ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera->ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera->ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera->ProcessKeyboard(ROTATE_LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera->ProcessKeyboard(ROTATE_RIGHT, deltaTime);

	// helpers

	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !key_pressed) {
		renderer_left->m_showNormals = !renderer_left->m_showNormals;
		renderer_right->m_showNormals = !renderer_right->m_showNormals;
		key_pressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE) {
		key_pressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		renderer_left->m_spinPointCloudLeft = false;
		renderer_left->m_spinPointCloudRight = true;
		renderer_right->m_spinPointCloudLeft = false;
		renderer_right->m_spinPointCloudRight = true;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		renderer_left->m_spinPointCloudLeft = true;
		renderer_left->m_spinPointCloudRight = false;
		renderer_right->m_spinPointCloudLeft = true;
		renderer_right->m_spinPointCloudRight = false;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		renderer_left->m_spinPointCloudLeft = false;
		renderer_left->m_spinPointCloudRight = false;
		renderer_right->m_spinPointCloudLeft = false;
		renderer_right->m_spinPointCloudRight = false;
	}
}

void App::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			left_mouse_pressed = true;
		}
		else if (action == GLFW_RELEASE)
		{
			left_mouse_pressed = false;
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS) {
			right_mouse_pressed = true;
		}
		else if (action == GLFW_RELEASE) {
			right_mouse_pressed = false;
		}
	}
}

void App::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	if(left_mouse_pressed){
		camera->ProcessMouseMovement(xoffset, yoffset);
	}
	else if (right_mouse_pressed)
		camera->ProcessMousePan(xoffset, yoffset);
}

void App::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera->ProcessMouseScroll(static_cast<float>(yoffset));
}