#pragma once

#include "Camera.h"
#include "Renderer.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include <iostream>



class App {

public:

	Camera* camera;

	float deltaTime = 0.0f;
	float lastFrame = 0.0f;
	bool firstMouse = true;
	float lastX = SCR_WIDTH / 2.0f;
	float lastY = SCR_HEIGHT / 2.0f;

	App(unsigned int width, unsigned height);
	~App();
	void run();
	void processInput(GLFWwindow* window);
	void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
private:
	GLFWwindow* window;
	Renderer* renderer;
};