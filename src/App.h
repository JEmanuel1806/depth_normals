#pragma once

#include "imgui.h";
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "Camera.h"
#include "Renderer.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include <sstream>
#include <string>
#include <iostream>



class App {

public:

	unsigned int width, height;

	Camera* camera;

	// ---------- Control Handling (Keyboard & Mouse) ------------- //
	float deltaTime = 0.0f;
	float lastFrame = 0.0f;
	bool firstMouse = true;
	bool key_pressed = false;
	bool left_mouse_pressed = false;
	bool right_mouse_pressed = false;
	float lastX = width;
	float lastY = height;
	
	App(unsigned int w, unsigned int h, std::string inputFile);
	~App();
	void run();
	void setupGUI(float fps);
	void processInput();
	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
private:
	GLFWwindow* window;
	Renderer* renderer;
};