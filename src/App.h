#pragma once

#include "Camera.h"
#include "Renderer.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include <sstream>
#include <string>


#include <iostream>



class App {

public:

	Camera* camera;

	float deltaTime = 0.0f;
	float lastFrame = 0.0f;
	bool firstMouse = true;
	bool key_pressed = false;
	bool left_mouse_pressed = false;
	bool right_mouse_pressed = false;
	float lastX = SCREEN_WIDTH / 2.0f;
	float lastY = SCREEN_HEIGHT / 2.0f;
	

	App(unsigned int width, unsigned height);
	~App();
	void run(unsigned int width, unsigned int height);
	void processInput();
	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
private:
	GLFWwindow* window;
	Renderer* renderer_left;
	Renderer* renderer_right;
};