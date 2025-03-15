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

	renderer = new Renderer();
	renderer->start();
}

App::~App() {
	delete renderer;
	glfwDestroyWindow(window);
	glfwTerminate();
}

void App::run() {
	while (!glfwWindowShouldClose(window)) {
		renderer->render(); 

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}