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

	camera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f));
	renderer = new Renderer(camera);
	renderer->start();

	
}

App::~App() {
	delete renderer;
	delete camera;
	glfwDestroyWindow(window);
	glfwTerminate();
}

void App::run() {

	while (!glfwWindowShouldClose(window)) {

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);


		renderer->render(); 

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
		renderer->show_normals = !renderer->show_normals;
		key_pressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE) {
		key_pressed = false;
	}
}



void App::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	/*
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

	camera.ProcessMouseMovement(xoffset, yoffset);
	*/
}