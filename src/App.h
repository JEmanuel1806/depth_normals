#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Renderer.h"

#include <iostream>

class App {

public:
	App(unsigned int width, unsigned height);
	~App();
	void run();
private:
	GLFWwindow* window;
	Renderer* renderer;
};