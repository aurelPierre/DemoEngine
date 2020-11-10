#include "GLFWWindowSystem.h"

#include "LogSystem.h"

#include <iostream>
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort

static void glfw_error_callback(int error, const char* description)
{
	LOG(ez::ERROR, std::string("Glfw Error ") + std::to_string(error) + ':' + description)
}

static void glfw_framebuffer_callback(GLFWwindow* window, int width, int height)
{
	LOG(ez::DEBUG, std::string("Resize callback is happening"))

	GLFWWindowData* windowData = (GLFWWindowData*)glfwGetWindowUserPointer(window);
	windowData->_width = width;
	windowData->_height = height;
	windowData->_shouldUpdate = true;
}

GLFWWindowData*	GLFWWindowSystem::CreateWindow()
{
	if (!glfwInit())
		return nullptr;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	_windowData._window = glfwCreateWindow(_windowData._width, _windowData._height, "Dear ImGui GLFW+Vulkan example", NULL, NULL);
	
	glfwSetWindowUserPointer(_windowData._window, this);
	glfwSetErrorCallback(glfw_error_callback);
	glfwSetFramebufferSizeCallback(_windowData._window, glfw_framebuffer_callback);

	return &_windowData;
}

void GLFWWindowSystem::DeleteWindow()
{
	glfwDestroyWindow(_windowData._window);
	glfwTerminate();
}

bool GLFWWindowSystem::UpdateInput()
{
	if (glfwWindowShouldClose(_windowData._window))
		return true;

	_windowData._shouldUpdate = false;
	glfwPollEvents();

	return false;
}